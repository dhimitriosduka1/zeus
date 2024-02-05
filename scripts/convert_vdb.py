import pyopenvdb as vdb
import numpy as np
import argparse

parser = argparse.ArgumentParser(description='Convert vdb to vol binary file for lightwave.')
parser.add_argument('source', metavar='src', type=str,
                    help='source .vdb file path')
parser.add_argument('destination', metavar='dst', type=str,
                    help='destination .vol file path')
parser.add_argument('--field', metavar='f', type=str, required=False, default="density",
                    help='desired vdb field to convert')       

args = parser.parse_args()

# Very slow and stupid way to convert VDB grid
def convert(filename, outputname, gridname):
    data = vdb.read(filename, gridname)

    bb = data.evalActiveVoxelBoundingBox()
    res = (bb[1][0] - bb[0][0] + 1, bb[1][1] - bb[0][1] + 1, bb[1][2] - bb[0][2] + 1)
    grid_list = np.empty((res[0] * res[1] * res[2] + 3), 'float32')
    grid_list[0] = res[0]
    grid_list[1] = res[1]
    grid_list[2] = res[2]
    
    print('loading grid...', filename, gridname, f'{res[0]} x {res[1]} x {res[2]}')
    max_v = 0
    for item in data.iterOnValues():
        item_pos = item.min
        item_pos = (item_pos[0] - bb[0][0], res[1] - 1 - (item_pos[1] - bb[0][1]), item_pos[2] - bb[0][2])
        item_idx = (item_pos[2] * res[0] * res[1]) + (item_pos[1] * res[0]) + item_pos[0]
        grid_list[item_idx + 3] = item.value
        if item.value > max_v:
            max_v = item.value

    print("writing grid...", outputname)
    with open(outputname, 'wb') as f:
        grid_list.tofile(f)
    print('done.')

# convert("./tests/volumes/cloud.vdb", "./tests/volumes/cloud.vol", "density")
convert(args.source, args.destination, args.field)
