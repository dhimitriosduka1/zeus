import bpy
from .utils import *
from .defaults import *
from .registry import SceneRegistry
from .xml_node import XMLNode


def _export_area_light(registry: SceneRegistry, instance_node, light: bpy.types.Light, inst: bpy.types.DepsgraphObjectInstance):
    # Compute actual matrix
    # From my understanding, object transforms in Blender are always similarity transformations.
    # This means we do not need to worry about the angle formed by the spanning vectors of our
    # area light when computing the area for power normalization, as they will still be orthogonal.
    scale_x = light.size
    if light.shape == "SQUARE" or light.shape == "DISK":
        scale_y = light.size
    elif light.shape == "RECTANGLE" or light.shape == "ELLIPSE":
        scale_y = light.size_y
    else:
        registry.warn(f"Unsupported light shape '{light.shape}'")
        scale_y = light.size
    
    matrix_world = [ [ x for x in row ] for row in inst.matrix_world ]
    lensqr_x = 0
    lensqr_y = 0
    for i in range(3):
        matrix_world[i][0] *= scale_x / 2
        matrix_world[i][1] *= scale_y / 2
        matrix_world[i][2] *= -1
        lensqr_x += matrix_world[i][0] ** 2
        lensqr_y += matrix_world[i][1] ** 2

    instance_node.add("shape", type="rectangle")
    transform = instance_node.add("transform")
    transform.add("matrix", value=str_flat_matrix(matrix_world))

    return 1 / (16 * (lensqr_x * lensqr_y) ** 0.5)

def _export_point_light(registry: SceneRegistry, instance_node, light: bpy.types.Light, inst: bpy.types.DepsgraphObjectInstance):
    radius = max(light.shadow_soft_size, 1e-3)

    instance_node.add("shape", type="sphere")
    transform = instance_node.add("transform")
    transform.add("scale", value=radius)
    transform.add("translate",
        x=inst.matrix_world[0][3],
        y=inst.matrix_world[1][3],
        z=inst.matrix_world[2][3])

    # I don't understand any of this either, but it's Blender's convention :-)
    return 1 / (4 * (3.14159 * radius) ** 2)

def export_light(registry: SceneRegistry, inst):
    light = inst.object.data
    if light.cycles.is_portal:
        registry.warn("Light portals are not supported")
        return []

    if light.type == "POINT" and light.shadow_soft_size < 1e-3:
        power = str_flat_array([ light.energy * light.color[chan] for chan in range(3) ])
        position = str_flat_array([ inst.matrix_world[dim][3] for dim in range(3) ])
        return [XMLNode("light", type="point", position=position, power=power)]

    if light.type == "SUN":
        intensity = str_flat_array([ light.energy * light.color[chan] for chan in range(3) ])
        position = str_flat_array([ inst.matrix_world[dim][2] for dim in range(3) ])
        return [XMLNode("light", type="directional", direction=position, intensity=intensity)]

    if registry.settings.enable_area_lights:
        light_node = XMLNode("light", type="area")
        instance_node = light_node.add("instance") # id=registry._make_unique_name(light.name)
    else:
        # Fallback if students did not implement area lights yet
        light_node = XMLNode("instance")
        instance_node = light_node

    if light.type == "POINT":
        normalization = _export_point_light(registry, instance_node, light, inst)
    elif light.type == "AREA":
        normalization = _export_area_light(registry, instance_node, light, inst)
    else:
        registry.warn(f"Light type {light.type} unsupported")
        return []

    emission = [
        normalization * light.energy * light.color[chan]
        for chan in range(3)
    ]

    instance_node.add("emission", type="lambertian").add(
        "texture", name="emission", type="constant", value=str_flat_array(emission))
    return [light_node]
