from . import exporter_ui, addon_preferences

bl_info = {
    "name": "Lightwave Scene",
    "author": "Ömercan Yazici, Alexander Rath",
    "description": "Export scene to Lightwave",
    "version": (0, 2, 5),
    "blender": (3, 0, 0),
    "location": "File > Import-Export",
    "category": "Import-Export",
    "support": "COMMUNITY",
}


def register():
    addon_preferences.register()
    exporter_ui.register()


def unregister():
    exporter_ui.unregister()
    addon_preferences.unregister()
