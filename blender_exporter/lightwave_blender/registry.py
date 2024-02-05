import bpy
import os
import re

from .utils import find_unique_name
from .xml_node import XMLNode


class Token(object):
    def __init__(self, id, name):
        self.name_full = id
        self.name = name
    
    def __contains__(self, key):
        return key == "name_full"

class SceneRegistry(object):
    def __init__(self, path, depsgraph, settings, op):
        self.names: set[str] = set()
        self.converted: dict[str, dict] = {}

        self.path = path
        self.depsgraph = depsgraph
        self.settings = settings
        self.operator = op
    
    def _make_unique_name(self, name: str):
        name = re.sub("[^a-zA-Z0-9_\\- ]", "_", name)
        return find_unique_name(self.names, name)

    def _build_reference(self, node: XMLNode, name: str):
        if "id" not in node.attributes:
            node.attributes["id"] = self._make_unique_name(name)
        kw = {}
        if "name" in node.attributes:
            kw["name"] = node.attributes["name"]
        kw["id"] = node.attributes["id"]
        return XMLNode("ref", **kw)

    def export(self, original: bpy.types.Object, export_fn):
        # TODO: full_name might not be unique
        full_name = original.name_full if "name_full" in original else original.name
        full_name += "/" + original.__class__.__name__
        if full_name in self.converted:
            conv = self.converted[full_name]
            if isinstance(conv, list):
                return [ self._build_reference(node, original.name) for node in conv ]
            return self._build_reference(conv, original.name)
        
        self.converted[full_name] = export_fn("TODO")
        return self.converted[full_name]
    
    @property
    def scene(self):
        return self.depsgraph.scene
    
    def debug(self, text: str):
        self.operator.report({'DEBUG'}, text)

    def warn(self, text: str):
        self.operator.report({'WARNING'}, text)

    def error(self, text: str):
        self.operator.report({'ERROR'}, text)
