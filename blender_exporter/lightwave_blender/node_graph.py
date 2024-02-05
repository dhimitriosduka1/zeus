import bpy
from .registry import SceneRegistry
from .utils import find_unique_name

class RMInput(object):
    def __init__(self, node_graph):
        self.value: any = None
        self.link: (str, str) = None
        self.node_graph: RMNodeGraph = node_graph
    
    def has_value(self):
        return self.value is not None

    def is_linked(self):
        return self.link is not None

    def linked_node(self):
        return self.node_graph.nodes[self.link[0]]
    
    def linked_output(self):
        return self.link[1]

class RMNode(object):
    def __init__(self, node_graph, node: bpy.types.Node):
        self.node_graph: RMNodeGraph = node_graph
        self.bl_node: bpy.types.Node = node
        self.links: dict[str, (str, str)] = {} # input identifier -> (from_node.name, from_output.name)
        self.values: dict[str, any] = {} # input identifier -> any

        for input in node.inputs.values():
            if input.is_linked:
                if len(input.links) != 1:
                    node_graph.registry.warn("Multi-input links are not supported!")
                    continue
                
                link = input.links[0]
                self.links[input.identifier] = (link.from_node.name, link.from_socket.identifier)
            
            if hasattr(input, "default_value"):
                value = input.default_value
                if not isinstance(value, (int, float, str)):
                    value = list(value)
                self.values[input.identifier] = value
    
    def input(self, name: str):
        input = RMInput(self.node_graph)
        input.value = self.values.get(name)
        input.link = self.links.get(name)
        return input

class RMNodeGraph(object):
    def __init__(self, registry: SceneRegistry, name: str, node_tree: bpy.types.NodeTree):
        self.name = name
        self.registry = registry
        self.nodes: dict[str, RMNode] = {} # node.name -> RMNode

        for node in node_tree.nodes.values():
            self.nodes[node.name] = RMNode(self, node)
    
    def delete_node(self, node_name):
        if node_name is None:
            return
        
        for (other_name, node) in self.nodes.items():
            for (input_name, link) in node.links.items():
                if link[0] == node_name:
                    self.registry.warn(f"cannot remove node '{node_name}' as '{other_name}'.'{input_name}' still relies on output '{link[1]}'!")
                    return
        
        del self.nodes[node_name]

    # @todo this could be made more efficient if the graph was double linked
    def replace_link(self, old_link, new_link):
        for node in self.nodes.values():
            for (inp, link) in list(node.links.items()):
                if link == old_link:
                    if new_link is None:
                        del node.links[inp]
                    else:
                        node.links[inp] = new_link
    
    def replace_link_with_value(self, old_link, value):
        for node in self.nodes.values():
            for (inp, link) in list(node.links.items()):
                if link == old_link:
                    del node.links[inp]
                    node.values[inp] = value # @todo casting?
    
    def apply_renaming(self, renaming: dict[str, str]):
        old_nodes = self.nodes
        self.nodes = {}

        for (old_name, new_name) in renaming.items():
            node = old_nodes[old_name]
            self.nodes[new_name] = node
            for (inp, (link_node, link_output)) in node.links.items():
                node.links[inp] = (renaming[link_node], link_output)

    def use_labels_as_names(self):
        renaming = {}
        used_names = set()
        for (old_name, node) in self.nodes.items():
            new_name = find_unique_name(used_names, node.bl_node.label or node.bl_node.name)
            renaming[old_name] = new_name
        self.apply_renaming(renaming)
    
    def avoid_names(self, used_names: set[str]):
        renaming = {}
        for old_name in self.nodes.keys():
            new_name = find_unique_name(used_names, old_name)
            renaming[old_name] = new_name
        self.apply_renaming(renaming)

    def remove_muted_nodes(self):
        for (node_name, node) in list(self.nodes.items()):
            if not node.bl_node.mute:
                continue
            
            self.registry.warn("Node muting has not been tested!")
            for output in node.bl_node.outputs.values():
                link = None
                for input in node.bl_node.inputs.values():
                    if input.type == output.type and input.is_linked:
                        link = node.links[input.identifier]
                        break
                
                self.replace_link((node_name, output.identifier), link)

    def remove_reroute_nodes(self):
        for (node_name, node) in list(self.nodes.items()):
            if isinstance(node.bl_node, bpy.types.NodeReroute):
                assert(len(node.bl_node.inputs) == 1)
                assert(len(node.bl_node.outputs) == 1)

                in_id = node.bl_node.inputs[0].identifier
                out_id = node.bl_node.outputs[0].identifier

                self.replace_link((node_name, out_id), node.links.get(in_id))
                self.delete_node(node_name)

    def remove_layout_nodes(self):
        for (node_name, node) in list(self.nodes.items()):
            if isinstance(node.bl_node, bpy.types.NodeFrame):
                self.delete_node(node_name)
    
    # @todo we do not support casting via GroupOutput yet
    # (e.g., a color A connected to a float GroupOutput B connected to a color input C causes the color
    # at C to become black and white, but since we directly connect A->C we do not get this effect)
    def inline_node_groups_recursively(self, max_depth=8):
        if max_depth == 0:
            self.registry.warn("Maximum depth reached while inlining node group")
            return
        
        for (node_name, node) in list(self.nodes.items()):
            if not isinstance(node.bl_node, bpy.types.ShaderNodeGroup):
                continue
            
            sub_graph = RMNodeGraph(self.registry, self.name, node.bl_node.node_tree)
            sub_graph.inline_node_groups_recursively(max_depth - 1)
            sub_graph.avoid_names(set(self.nodes.keys()))

            groupinputs: list[str] = []
            groupoutput: str = None

            for (sub_node_name, sub_node) in sub_graph.nodes.items():
                if isinstance(sub_node.bl_node, bpy.types.NodeGroupInput):
                    groupinputs.append(sub_node_name)
                    continue
                elif isinstance(sub_node.bl_node, bpy.types.NodeGroupOutput):
                    if sub_node.bl_node.is_active_output:
                        groupoutput = sub_node_name
                        # we keep this node, because its outputs might be referenced and we want
                        # replace_link to update this node
                    else:
                        continue

                assert(sub_node_name not in self.nodes)
                self.nodes[sub_node_name] = sub_node
            
            for input in node.bl_node.inputs.values():
                for groupinput in groupinputs:
                    if input.identifier in node.links:
                        self.replace_link((groupinput, input.identifier), node.links[input.identifier])
                    else:
                        self.replace_link_with_value((groupinput, input.identifier), node.values[input.identifier])
            
            for output in node.bl_node.outputs.values():
                if groupoutput is not None:
                    onode = sub_graph.nodes[groupoutput]
                    if output.identifier in onode.links:
                        self.replace_link((node_name, output.identifier), onode.links[output.identifier])
                    else:
                        self.replace_link_with_value((node_name, output.identifier), onode.values[output.identifier])
                else:
                    self.replace_link((node_name, output.identifier), None)
            
            self.delete_node(groupoutput)
            self.delete_node(node_name)