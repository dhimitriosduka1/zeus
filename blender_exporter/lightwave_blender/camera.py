import math
from .utils import *
from .registry import SceneRegistry
from .xml_node import XMLNode


def export_camera(registry: SceneRegistry):
    camera = registry.scene.camera
    if camera is None:
        registry.error("Your scene needs a camera!")
        return []

    matrix = orient_camera(camera.matrix_world, skip_scale=True)

    render = registry.scene.render
    res_x = int(render.resolution_x * render.resolution_percentage * 0.01)
    res_y = int(render.resolution_y * render.resolution_percentage * 0.01)

    # TODO: Other types?
    camera_node = XMLNode("camera", type="perspective")

    camera_node.add("integer", name="width", value=res_x)
    camera_node.add("integer", name="height", value=res_y)

    camera_node.add("float", name="fov", value=math.degrees(
        2 * math.atan(camera.data.sensor_width / (2 * camera.data.lens))))
    camera_node.add("string", name="fovAxis", value="x" if render.resolution_x > render.resolution_y else "y")

    # camera_node.add("float", name="nearClip", value=camera.data.clip_start)
    # camera_node.add("float", name="farClip", value=camera.data.clip_end)

    camera_node.add("transform").add("matrix", value=str_flat_matrix(matrix))

    return [camera_node]
