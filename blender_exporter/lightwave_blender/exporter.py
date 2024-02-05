import bpy
import os

from .light import export_light
from .shape import export_shape
from .camera import export_camera
from .materials import export_material, export_default_bsdf
from .utils import *
from .defaults import *
from .addon_preferences import get_prefs
from .xml_node import XMLNode, XMLRootNode
from .registry import SceneRegistry
from .world import export_world_background


def export_technique(registry: SceneRegistry):
    if getattr(registry.scene, 'cycles', None) is not None and bpy.context.engine == 'CYCLES':
        cycles = registry.scene.cycles
        max_depth = cycles.max_bounces
        clamp = max(cycles.sample_clamp_direct, cycles.sample_clamp_indirect)
        spp = cycles.samples
    elif getattr(registry.scene, 'eevee', None) is not None and bpy.context.engine == 'BLENDER_EEVEE':
        eevee = registry.scene.eevee
        max_depth = eevee.gi_diffuse_bounces
        clamp = eevee.gi_glossy_clamp
        spp = eevee.taa_render_samples
    else:
        max_depth = 10
        clamp = 0
        spp = 64

    pt = XMLNode("integrator", type="pathtracer", depth=max_depth, nee="true", mis="true")
    pt.add("ref", id="scene")
    pt.add("image", id="noisy")
    pt.add("sampler", type="independent", count=spp)

    if False:
        alb = XMLNode("integrator", type="albedo")
        alb.add("ref", id="scene")
        alb.add("image", id="albedo")
        alb.add("sampler", type="halton", count=32)

        norm = XMLNode("integrator", type="normals")
        norm.add("ref", id="scene")
        norm.add("image", id="normals")
        norm.add("sampler", type="halton", count=32)

        norm = XMLNode("postprocess", type="denoising")
        norm.add("ref", name="input", id="noisy")
        norm.add("ref", name="albedo", id="albedo")
        norm.add("ref", name="normals", id="normals")
        norm.add("image", id="denoised")
    
    return [pt]


def export_entity(registry: SceneRegistry, inst, shape: XMLNode, mat_id: int) -> XMLNode:
    instance_node = XMLNode("instance")

    inst_mat = None
    if registry.settings.export_materials:
        if mat_id < len(inst.object.material_slots):
            inst_mat = inst.object.material_slots[mat_id].material
            if inst_mat is not None:
                instance_node.add_children(registry.export(inst_mat, lambda unique_name: export_material(registry, inst_mat)))
            else:
                registry.warn(f"Obsolete material slot {mat_id} with instance {inst.object.data.name}. Maybe missing a material?")
                instance_node.add_children(export_default_bsdf())
        else:
            #registry.warn(f"Entity {inst.object.name} has no material")
            instance_node.add_children(export_default_bsdf())
    else:
        instance_node.add_children(export_default_bsdf())

    instance_node.add_child(shape)
    instance_node.add("transform").add("matrix", value=str_flat_matrix(inst.matrix_world))
    
    return instance_node

def export_objects(registry: SceneRegistry):
    # Export all given objects
    result = []

    # Export entities & shapes
    for inst in registry.depsgraph.object_instances:
        object_eval = inst.object
        if object_eval is None:
            continue
        if registry.settings.use_selection and not object_eval.original.select_get():
            continue
        if not registry.settings.use_selection and not inst.show_self:
            continue

        objType = object_eval.type
        if objType in {'MESH', 'CURVE', 'SURFACE', 'META', 'FONT', 'CURVES'}:
            shapes: list[XMLNode] = registry.export(object_eval.original.data,
                lambda unique_name: export_shape(registry, object_eval))
            if len(shapes) == 0:
                registry.warn(f"Entity {object_eval.name} has no material or shape and will be ignored")

            for (mat_id, shape) in enumerate(shapes):
                result.append(export_entity(registry, inst, shape, mat_id))
        elif objType == "LIGHT" and registry.settings.export_lights:
            result += export_light(registry, inst)
    
    return result


def export_scene(op, filepath, context, settings):
    depsgraph = context.evaluated_depsgraph_get() if not isinstance(
        context, bpy.types.Depsgraph) else context

    # Root
    root = XMLRootNode()
    scene = XMLNode("scene", id="scene")

    # Create a path for meshes & textures
    rootPath = os.path.dirname(filepath)
    meshDir = os.path.join(rootPath, get_prefs().mesh_dir_name)
    texDir = os.path.join(rootPath, get_prefs().tex_dir_name)
    os.makedirs(meshDir, exist_ok=True)
    os.makedirs(texDir, exist_ok=True)

    registry = SceneRegistry(rootPath, depsgraph, settings, op)
    scene.add_children(export_camera(registry))
    scene.add_children(export_objects(registry))

    if settings.enable_background:
        scene.add_children(export_world_background(registry, depsgraph.scene))

    root.add_child(scene)
    root.add_children(export_technique(registry))

    # Remove mesh & texture directory if empty
    try:
        if len(os.listdir(meshDir)) == 0:
            os.rmdir(meshDir)
        if len(os.listdir(texDir)) == 0:
            os.rmdir(texDir)
    except:
        pass  # Ignore any errors

    return root


def export_scene_to_file(op, filepath, context, settings):
    root = export_scene(op, filepath, context, settings)

    # Write the result into a file
    with open(filepath, 'w') as fp:
        fp.write(root.dump())
