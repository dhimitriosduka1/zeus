from .utils import str_float

class XMLNode:
    def __init__(self, node_name, **attributes):
        self.name = node_name
        self.attributes = attributes
        self.children = []

    def set_name(self, name: str):
        self.attributes["name"] = name

    def add_child(self, child, name=None):
        if name is not None:
            child.set_name(name)
        self.children.append(child)
    
    def add_children(self, children):
        self.children += children

    def add(self, node_name, **attributes):
        child = XMLNode(node_name, **attributes)
        self.add_child(child)
        return child

    def dump(self, ident=0):
        # Start tag
        output = "  " * ident
        output += f"<{self.name}"

        # Attributes
        priority = ["name", "id"]
        sorted_attr = sorted(self.attributes.keys(), key=lambda attr: priority.index(attr) if attr in priority else len(priority))
        for attr_name in sorted_attr:
            attr_value = self.attributes[attr_name]
            if attr_value is not None:
                if isinstance(attr_value, bool):
                    attr_value = "true" if attr_value else "false"
                elif isinstance(attr_value, float):
                    attr_value = str_float(attr_value)
                output += f" {attr_name}=\"{attr_value}\""

        if len(self.children) > 0:
            output += ">\n"

            # Children
            for child in self.children:
                output += child.dump(ident+1) + "\n"

            # Close
            output += "  " * ident
            output += f"</{self.name}>"
        else:
            output += "/>"

        return output


class XMLRootNode:
    def __init__(self):
        self.children = []

    def add_child(self, child):
        self.children.append(child)
    
    def add_children(self, children):
        self.children += children

    def add(self, node_name, **attributes):
        child = XMLNode(node_name, **attributes)
        self.add_child(child)
        return child

    def dump(self):
        # Children
        output = ""
        for child in self.children:
            output += child.dump(0) + "\n"
        return output
