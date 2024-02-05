import bpy
import os

from .utils import *
from .addon_preferences import get_prefs
from .xml_node import XMLNode
from .registry import SceneRegistry, Token
from .node_graph import RMInput


def _export_fallback():
    return XMLNode("texture", type="constant", value=0)

def _export_default(registry: SceneRegistry, input: RMInput, exposure):
    if not input.has_value():
        return _export_fallback()

    factor = exposure or 1
    value = input.value
    if isinstance(value, float):
        value *= factor
    elif isinstance(value, list):
        value = [ v * factor for v in value[0:3] ]
    return XMLNode("texture", type="constant", value=str_flat_array(value))

def _export_scalar_value(registry: SceneRegistry, input: RMInput, exposure):
    factor = exposure or 1
    return XMLNode("texture", type="constant", value=input.linked_node().bl_node.outputs[0].default_value * factor)

def _export_rgb_value(registry: SceneRegistry, input: RMInput, exposure):
    factor = exposure or 1
    node = input.linked_node().bl_node
    default_value = [ v * factor for v in node.outputs[0].default_value[0:3] ]
    return XMLNode("texture", type="constant", value=str_flat_array(default_value))

def _export_image(registry: SceneRegistry, image, path, is_f32=False, keep_format=False):
    if os.path.exists(path) and not registry.settings.overwrite_existing_textures:
        return

    # Make sure the image is loaded to memory, so we can write it out
    if not image.has_data:
        image.pixels[0]

    # Export the actual image data
    try:
        old_path = image.filepath_raw
        old_format = image.file_format
        try:
            image.filepath_raw = path
            if not keep_format:
                image.file_format = "PNG" if not is_f32 else "OPEN_EXR"
            image.save()
        finally:  # Never break the scene!
            image.filepath_raw = old_path
            image.file_format = old_format
    except:
        if not keep_format:
            raise RuntimeError(
                "Can not change the original format of given image")
        # Try other way
        image.save_render(path)


def _handle_image(registry: SceneRegistry, image: bpy.types.Image):
    tex_dir_name = get_prefs().tex_dir_name

    if image.source == 'GENERATED':
        img_name = image.name + \
            (".png" if not image.use_generated_float else ".exr")
        img_path = os.path.join(tex_dir_name, img_name)
        _export_image(registry, image, os.path.join(registry.path, img_path), is_f32=image.use_generated_float)
        return img_path.replace('\\', '/') # Ensure the image path is not using \ to keep the xml valid
    elif image.source == 'FILE':
        filepath = image.filepath_raw if image.filepath_raw is not None else image.filepath
        img_path = bpy.path.abspath(bpy.path.resolve_ncase(filepath), library=image.library)
        try:
            img_path = bpy.path.relpath(img_path, start=registry.path)
        except:
            # relpath can fail on Windows if paths have different drive letters
            print("unable to create relative path")
        img_path = img_path.replace("\\", "/")
        if img_path.startswith("//"):
            img_path = img_path[2:]

        copy_image = image.packed_file or getattr(
            registry.settings, "copy_images", False) or img_path == ''

        if copy_image:
            registry.debug(f"Copying image {img_path}") 
            img_name = bpy.path.basename(img_path)

            # Special case: We can not export PNG if bit depth is not 8 (or 32), for whatever reason
            if img_name == '' or image.depth > 32 or image.depth == 16:
                keep_format = False
                if image.depth > 32 or image.depth == 16 or image.file_format in ["OPEN_EXR", "OPEN_EXR_MULTILAYER", "HDR"]:
                    is_f32 = True
                    extension = ".exr"
                else:
                    is_f32 = False
                    extension = ".png"
                img_path = os.path.join(tex_dir_name, image.name + extension)
            else:
                keep_format = True
                is_f32 = False  # Does not matter
                img_path = os.path.join(tex_dir_name, img_name)

            try:
                _export_image(registry, image, os.path.join(registry.path, img_path),
                                is_f32=is_f32, keep_format=keep_format)
            except:
                # Above failed, so give this a try
                img_path = os.path.join(tex_dir_name, img_name)
                _export_image(registry, image, os.path.join(registry.path, img_path),
                                  is_f32=False, keep_format=True)
        return img_path.replace('\\', '/') # Ensure the image path is not using \ to keep the xml valid
    else:
        registry.error(f"Image type {image.source} not supported")
        return None


def _export_image_texture(registry: SceneRegistry, input: RMInput, exposure):
    bl_node = input.linked_node().bl_node
    if not bl_node.image:
        registry.error(f"Image node {bl_node.name} has no image")
        return _export_fallback()

    def export(unique_name):
        img_path = _handle_image(registry, bl_node.image)
        kw = {}

        if exposure is not None and exposure != 1:
            kw["exposure"] = exposure
        
        if bl_node.extension == "EXTEND" or bl_node.extension == "CLIP":
            kw["border"] = "clamp"

        if bl_node.interpolation == "Closest":
            kw["filter"] = "nearest"

        cs = bl_node.image.colorspace_settings.name
        if cs == "Linear" or cs == "Non-Color" or cs == "Raw":
            kw["linear"] = True
        elif cs == "sRGB":
            pass
        else:
            registry.error(f"Unsupported color space {cs}")

        return XMLNode("texture", type="image", filename=img_path, **kw)
    
    return registry.export(Token(str(bl_node.image.as_pointer()), bl_node.name), export)


def _export_env_image_texture(registry: SceneRegistry, input: RMInput, exposure):
    # Handle it the same way as standard textures, but with tiny differences (e.g., fixed border handling)
    bl_node = input.linked_node().bl_node
    if not bl_node.image:
        registry.error(f"Image node {bl_node.name} has no image")
        return _export_fallback()

    def export(unique_name):
        img_path = _handle_image(registry, bl_node.image)
        kw = {}

        if exposure is not None and exposure != 1:
            kw["exposure"] = exposure
        
        kw["border"] = "clamp"

        if bl_node.interpolation == "Closest":
            kw["filter"] = "nearest"

        cs = bl_node.image.colorspace_settings.name
        if cs == "Linear" or cs == "Non-Color" or cs == "Raw":
            kw["linear"] = True
        elif cs == "sRGB":
            pass
        else:
            registry.error(f"Unsupported color space {cs}")

        return XMLNode("texture", type="image", filename=img_path, **kw)
    
    return registry.export(Token(str(bl_node.image.as_pointer()), bl_node.name), export)

def _export_normal_map(registry: SceneRegistry, input: RMInput, exposure, **args):
    return export_node(registry, input.linked_node().input("Color"), exposure, **args) # TODO: Strength

_node_handlers: dict[str, any] = {
    "ShaderNodeTexImage": _export_image_texture,
    "ShaderNodeTexEnvironment": _export_env_image_texture,
    "ShaderNodeValue": _export_scalar_value,
    "ShaderNodeRGB": _export_rgb_value,
    "ShaderNodeNormalMap": _export_normal_map,
}

def export_node(registry: SceneRegistry, input: RMInput, exposure=None) -> XMLNode:
    if not input.is_linked():
        return _export_default(registry, input, exposure)
    
    node = input.linked_node()
    for (typename, handler) in _node_handlers.items():
        if hasattr(bpy.types, typename) and isinstance(node.bl_node, getattr(bpy.types, typename)):
            return registry.export(Token(str(node.bl_node.as_pointer()), node.bl_node.name),
                                   lambda unique_name: handler(registry, input, exposure))
    
    registry.error(f"Material {node.node_graph.name} has a node of type {type(node.bl_node).__name__} which is not supported")
    return _export_fallback()
