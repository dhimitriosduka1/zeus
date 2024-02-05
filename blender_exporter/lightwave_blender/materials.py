import bpy
import math
import traceback

from .registry import SceneRegistry
from .node import export_node
from .utils import *
from .xml_node import XMLNode
from .node_graph import RMInput, RMNode, RMNodeGraph


def export_default_bsdf():
    node = XMLNode("bsdf", type="diffuse")
    node.add("texture", name="albedo", type="constant", value=0.8)
    return [node]

def _export_diffuse_bsdf(registry: SceneRegistry, bsdf_node: RMNode):
    node = XMLNode("bsdf", type="diffuse")
    node.add_child(export_node(registry, bsdf_node.input("Color")), name="albedo")
    return [node]

def _export_glass_bsdf_helper(registry: SceneRegistry, bsdf_node: RMNode, has_reflectance: bool):
    has_roughness = bsdf_node.bl_node.distribution != 'SHARP'
    node = XMLNode("bsdf", type="roughdielectric" if has_roughness else "dielectric")
    node.add_child(export_node(registry, bsdf_node.input("IOR")), name="ior")
    node.add_child(export_node(registry, bsdf_node.input("Color")), name="transmittance")
    
    if has_reflectance:
        node.add_child(export_node(registry, bsdf_node.input("Color")), name="reflectance")
    else:
        node.add("texture", name="reflectance", type="constant", value=0)
    
    if has_roughness:
        node.add_child(export_node(registry, bsdf_node.input("Roughness")), name="roughness")
    return [node]

def _export_glass_bsdf(registry: SceneRegistry, bsdf_node: RMNode):
    return _export_glass_bsdf_helper(registry, bsdf_node, has_reflectance=True)

def _export_refraction_bsdf(registry: SceneRegistry, bsdf_node: RMNode):
    return _export_glass_bsdf_helper(registry, bsdf_node, has_reflectance=False)

def _export_transparent_bsdf(registry: SceneRegistry, bsdf_node: RMNode):
    node = XMLNode("bsdf", type="dielectric")
    node.add("texture", name="ior", type="constant", value=1)
    node.add("texture", name="reflectance", type="constant", value=0)
    node.add_child(export_node(registry, bsdf_node.input("Color")), name="transmittance")
    return [node]

def _export_glossy_bsdf(registry: SceneRegistry, bsdf_node: RMNode):
    node = XMLNode("bsdf", type="principled")
    node.add_child(export_node(registry, bsdf_node.input("Color")), name="baseColor")
    node.add_child(export_node(registry, bsdf_node.input("Roughness")), name="roughness")
    node.add("texture", name="metallic", type="constant", value=1)
    node.add("texture", name="specular", type="constant", value=1)
    return [node]

def _is_black(v: list):
    if isinstance(v, float):
        return v == 0
    return all((c == 0 for c in v[0:3]))

def _export_emission_helper(registry: SceneRegistry, color: RMInput, strength: RMInput):
    if not color.is_linked():
        if not color.has_value() or _is_black(color.value):
            return []
    
    emission_scale = 1
    if strength.is_linked():
        registry.error("Only constant values for emission strength are supported")
    elif strength.has_value():
        emission_scale = float(strength.value)
    if emission_scale == 0:
        return []

    emission = XMLNode("emission", type="lambertian")
    emission.add_child(export_node(registry, color, exposure=emission_scale), name="emission")
    return [emission]

def _export_principled_bsdf(registry: SceneRegistry, bsdf_node: RMNode):
    node = XMLNode("bsdf", type="principled")

    for (lw_name, bl_name) in [
        ("baseColor", "Base Color"),
        ("roughness", "Roughness"),
        #("subsurface", "Subsurface"),
        ("metallic", "Metallic"),
        ("specular", "Specular"),
        #("specularTint", "Specular Tint"),
        #("transmission", "Transmission"),
        #("anisotropic", "Anisotropic"),
        #("sheen", "Sheen"),
        #("sheenTint", "Sheen Tint"),
        #("clearcoat", "Clearcoat"),
        #("clearcoatRoughness", "Clearcoat Roughness"),
        #("ior", "IOR"),
    ]:
        node.add_child(export_node(registry, bsdf_node.input(bl_name)), name=lw_name)
    
    emission = _export_emission_helper(registry, bsdf_node.input("Emission"), bsdf_node.input("Emission Strength"))
    return [node] + emission

def _export_emission(registry: SceneRegistry, bsdf_node: RMNode):
    return _export_emission_helper(registry, bsdf_node.input("Color"), bsdf_node.input("Strength"))

_bsdf_handlers: dict[str, any] = {
    "ShaderNodeBsdfDiffuse": _export_diffuse_bsdf,
    "ShaderNodeBsdfGlass": _export_glass_bsdf,
    "ShaderNodeBsdfRefraction": _export_refraction_bsdf,
    "ShaderNodeBsdfTransparent": _export_transparent_bsdf,
    "ShaderNodeBsdfGlossy": _export_glossy_bsdf,
    "ShaderNodeBsdfPrincipled": _export_principled_bsdf,
    "ShaderNodeEmission": _export_emission,
    "ShaderNodeBackground": _export_emission,
}

# @todo material type should be 'Material | World | Light'
def export_material(registry: SceneRegistry, material: bpy.types.Material):
    if not material.use_nodes:
        if isinstance(material, bpy.types.Light):
            return [] # TODO
        elif isinstance(material, bpy.types.World):
            return [] # TODO
        elif isinstance(material, bpy.types.Material):
            return export_default_bsdf() # TODO
        else:
            registry.error("Unsupported use of node trees")
            return []
    
    try:
        node_graph = RMNodeGraph(registry, material.name_full, material.node_tree)
        node_graph.inline_node_groups_recursively()
        node_graph.remove_reroute_nodes()
        node_graph.remove_muted_nodes()
        node_graph.remove_layout_nodes()

        for (node_name, rm_node) in node_graph.nodes.items():
            node = rm_node.bl_node
            if isinstance(node, bpy.types.ShaderNodeOutputMaterial) \
            or isinstance(node, bpy.types.ShaderNodeOutputWorld) \
            or isinstance(node, bpy.types.ShaderNodeOutputLight):
                if node.is_active_output:
                    return _export_bsdf(registry, rm_node.input("Surface"))
    except Exception as e:
        print(f"failed to export material {material.name}")
        print(e)
        traceback.print_exc()
    
    return [] # No active output

def _export_bsdf(registry: SceneRegistry, input: RMInput):
    if not input.is_linked():
        return [] # black bsdf

    bsdf_node = input.linked_node()
    
    if bsdf_node is None:
        registry.error(f"Material {input.node_graph.name} has no valid bsdf")
        return []
    
    result = []
    for (typename, handler) in _bsdf_handlers.items():
        if hasattr(bpy.types, typename) and isinstance(bsdf_node.bl_node, getattr(bpy.types, typename)):
            result += handler(registry, bsdf_node) # registry.export(bsdf_node.bl_node, lambda unique_name: handler(registry, bsdf_node))
            break
    else:
        # treat as emission
        emission = XMLNode("emission", type="lambertian")
        emission.add_child(export_node(registry, input), name="emission")
        result.append(emission)
    
    if (normal := bsdf_node.input("Normal")).is_linked():
        normal_node = export_node(registry, normal)
        normal_node.set_name("normal")
        result.append(normal_node)

    if (alpha := bsdf_node.input("Alpha")).is_linked() or (alpha.has_value() and alpha.value != 1):
        alpha_node = export_node(registry, alpha)
        alpha_node.set_name("alpha")
        result.append(alpha_node)
    
    return result
