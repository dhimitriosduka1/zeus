import bpy

from bpy.props import (
    StringProperty
)
from bpy.types import AddonPreferences

package_name = __import__(__name__.split('.')[0])


class LightwavePreferences(AddonPreferences):
    bl_idname = package_name.__package__

    mesh_dir_name: StringProperty(
        name="Mesh Dir",
        description="Name for directory containing meshes",
        default="meshes",
    )
    tex_dir_name: StringProperty(
        name="Texture Dir",
        description="Name for directory containing textures",
        default="textures",
    )

    def draw(self, context):
        layout = self.layout

        col = layout.column(heading="Directory Names")
        col.prop(self, "mesh_dir_name", text="Mesh")
        col.prop(self, "tex_dir_name", text="Texture")


def register():
    bpy.utils.register_class(LightwavePreferences)


def unregister():
    bpy.utils.unregister_class(LightwavePreferences)


def get_prefs() -> LightwavePreferences:
    return bpy.context.preferences.addons[package_name.__package__].preferences
