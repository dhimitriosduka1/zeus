# Lightwave Blender Plugin

The Lightwave blender exporter enables users to export scenes from blender to a format compatible with Lightwave.
The scene supported by the plugin include: 
 - Objects (reduced to triangle meshes)
 - Camera (perspective and orthogonal, only the active camera will be exported)
 - Lights (point light, spot light, directional light, area light, background)
 - Materials with extensive but not complete node support

To build the plugin simply compile the project or run build_plugin and add the generated zip file to Blender.

The plugin ´ignis_blender´ from the [Ignis](https://github.com/PearCoding/Ignis) project by Ömercan Yazici was used as a initial base for the plugin.
