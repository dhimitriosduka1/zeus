import os
import bmesh

from .addon_preferences import get_prefs
from .registry import SceneRegistry
from .xml_node import XMLNode
from .materials import export_material


def get_shape_name_base(obj, inst):
    modifiers = [mod.type for mod in obj.original.modifiers]
    has_nodes = 'NODES' in modifiers

    if has_nodes:
        # Not sure how to ensure shapes with nodes are handled as uniques
        # TODO: We better join them by material
        id = hex(inst.random_id).replace("0x", "").replace('-', 'M').upper()
        return f"{obj.name}_{id}"

    try:
        return f"{obj.data.name}-shape"
    except:
        return f"{obj.original.data.name}-shape"  # We use the original mesh name!


def _shape_name_material(name, mat_id):
    return f"_m_{mat_id}_{name}"

def _export_bmesh_by_material(registry: SceneRegistry, me) -> list[(str, str)]:
    mat_count = len(me.materials)
    shapes = []

    def _export_for_mat(mat_id, abs_filepath):
        from .ply import save_mesh as ply_save

        bm = bmesh.new()
        bm.from_mesh(me)

        # remove faces with other materials
        if mat_count > 1:
            for f in bm.faces:
                # Remove irrelevant faces
                # Special case: Assign invalid material indices to the last material 
                if f.material_index != mat_id and not ((f.material_index < 0 or f.material_index >= mat_count) and mat_id == mat_count-1):
                    bm.faces.remove(f)

        if len(bm.verts) == 0 or len(bm.faces) == 0 or not bm.is_valid:
            bm.free()
            return False

        # Make sure all faces are convex
        bmesh.ops.connect_verts_concave(bm, faces=bm.faces)
        bmesh.ops.triangulate(bm, faces=bm.faces)

        bm.normal_update()

        ply_save(
            filepath=abs_filepath,
            bm=bm,
            use_ascii=False,
            use_normals=True,
            use_uv=True,
            use_color=False
        )

        bm.free()
        return True
    
    if mat_count == 0:
        # special case if the mesh has no slots available
        mat_count = 1
    
    for mat_id in range(0, mat_count):
        shape_name = me.name if mat_count <= 1 else _shape_name_material(me.name, mat_id)
        rel_filepath = os.path.join(get_prefs().mesh_dir_name, shape_name + ".ply")
        abs_filepath = os.path.join(registry.path, rel_filepath)

        if os.path.exists(abs_filepath) and not registry.settings.overwrite_existing_meshes:
            # file is already exported
            pass
        elif _export_for_mat(mat_id, abs_filepath):
            # export successful
            pass
        else:
            # export failed
            continue
        
        shapes.append(rel_filepath.replace('\\', '/')) # Ensure the shape path is not using \ to keep the xml valid
    
    return shapes


def export_shape(registry: SceneRegistry, obj) -> list[XMLNode]:
    # TODO: We want the mesh to be evaluated with renderer (or viewer) depending on user input
    # This is not possible currently, as access to `mesh_get_eval_final` (COLLADA) is not available
    # nor is it possible to setup via dependency graph, see https://devtalk.blender.org/t/get-render-dependency-graph/12164
    try:
        me = obj.to_mesh(preserve_all_data_layers=False, depsgraph=registry.depsgraph)
    except RuntimeError as e:
        registry.error(f"Could not convert to mesh: {str(e)}")
        return []

    shapes = _export_bmesh_by_material(registry, me)
    obj.to_mesh_clear()

    return [ XMLNode("shape", type="mesh", filename=filepath) for filepath in shapes ]
