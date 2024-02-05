import mathutils
import re


def find_unique_name(used: set[str], name: str):
    unique_name = name
    index = 0

    while unique_name in used:
        unique_name = f"{name}.{index:03d}"
        index += 1
    
    used.add(unique_name)
    return unique_name


def escape_identifier(name):
    return re.sub('[^a-zA-Z0-9_]', '_', name)


def flat_matrix(matrix):
    return [x for row in matrix for x in row]


def str_float(f: float):
    return "%.5g" % f


def str_flat_matrix(matrix):
    return ",  ".join([
        ",".join([ str_float(v) for v in row ])
        for row in matrix
    ])


def str_flat_array(array):
    if isinstance(array, float):
        return str_float(array)
    return ",".join([str_float(x) for x in array])


def orient_camera(matrix, skip_scale=False):
    # Y Up, Front -Z
    loc, rot, sca = matrix.decompose()
    return mathutils.Matrix.LocRotScale(loc, rot @ mathutils.Quaternion((0, 0, 1, 0)) @ mathutils.Quaternion((0, 0, 0, 1)), mathutils.Vector.Fill(3, 1) if skip_scale else sca)


def try_extract_node_value(value, default=0):
    try:
        return float(value)
    except:
        return default


def check_socket_if_constant(socket, value):
    if socket.is_linked:
        return False

    if socket.type == "RGBA" or socket.type == "VECTOR":
        return socket.default_value[0] == value and socket.default_value[1] == value and socket.default_value[2] == value
    else:
        return socket.default_value == value


def check_socket_if_black(socket):
    return check_socket_if_constant(socket, value=0)


def check_socket_if_white(socket):
    return check_socket_if_constant(socket, value=1)
