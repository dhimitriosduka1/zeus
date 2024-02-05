import pyopenvdb as vdb
import subprocess
import os
import re
import numpy as np
import cv2
import math
from time import sleep

os.environ["OPENCV_IO_ENABLE_OPENEXR"]="1"

root_path = os.path.relpath(os.path.dirname(__file__), os.path.curdir)
build_path = os.path.join(root_path, "build")
with open(os.path.join(root_path, "CMakeLists.txt")) as f:
    binary_name = re.search(r"set\s*\(MY_TARGET_NAME\s+([^)\s]+)\s*\)", f.read(), re.IGNORECASE)[1]
    lightwave_path = os.path.join(build_path, binary_name)


def convert2(filename, outputname, outputname2):
    # grids = vdb.readAllGridMetadata(filename)

    # print("Found grids:")
    # for grid in grids:
    #     print(grid.name)

    data = vdb.read(filename, "density")
    data2 = vdb.read(filename, "flames")


    # bb = data.evalActiveVoxelBoundingBox()
    res = (bb_max[0] + 1, bb_max[1] + 1, bb_max[2] + 1)
    return res
    # res = (200, 200, 200)
    print('Resolution:', res)
    grid_list = [0 for i in range(res[0] * res[1] * res[2])]
    grid_list2 = [0 for i in range(res[0] * res[1] * res[2])]

    print(data.background)
    print('loading grid...', filename)
    max_v = 0
    for item in data.iterOnValues():  # read/write iterator
        item_pos = item.min
        item_pos = (item_pos[0], res[1] - 1 - (item_pos[1]), item_pos[2])
        item_idx = (item_pos[2] * res[0] * res[1]) + (item_pos[1] * res[0]) + item_pos[0]
        grid_list[item_idx] = item.value
        if item.value > max_v:
            max_v = item.value

    max_v2 = 0
    for item in data2.iterOnValues():  # read/write iterator
        item_pos = item.min
        item_pos = (item_pos[0], res[1] - 1 - (item_pos[1]), item_pos[2])
        item_idx = (item_pos[2] * res[0] * res[1]) + (item_pos[1] * res[0]) + item_pos[0]
        grid_list2[item_idx] = item.value
        if item.value > max_v2:
            max_v2 = item.value

    print("writing grid...")
    with open(outputname, "w+") as f:
        f.write(f'{res[0]} {res[1]} {res[2]}\n')
        for value in grid_list:
            f.write(f'{(value / max_v):.8f} ')
    print("writing grid...")
    with open(outputname2, "w+") as f:
        f.write(f'{res[0]} {res[1]} {res[2]}\n')
        for value in grid_list2:
            f.write(f'{(value / max_v2):.8f} ')
    print('done.')
    return res

def render(idx):
    input_name = f"./tests/volumes/smallCampfireVDB/smallCampfire_{idx:04}.vdb"
    output_name = f"./tests/volumes/campfire/density{idx}.txt"
    output_name2 = f"./tests/volumes/campfire/flames{idx}.txt"

    res = convert2(input_name, output_name, output_name2)

    with open('./template.xml', 'rt') as f:
        template = f.read()
        template = template.replace('<scale x="0" y="0" z="0"/>', f'<scale x="{res[0]}" y="{res[1]}" z="{res[2]}"/>')
        # Animation
        template = template.replace('tests/volumes/campfire/density135.txt', f'tests/volumes/campfire/density{idx}.txt')
        template = template.replace('tests/volumes/campfire/flames135.txt', f'tests/volumes/campfire/flames{idx}.txt')
        template = template.replace('volumes0', f'volumes_new{idx}')
        template = template.replace('seed="0"', f'seed="{idx * 105 + 173}"')
        
        

        


        # template = template.replace('angle="0" />', f'angle="{idx * 2 - 10}" />')
        # template = template.replace('z="-4" x="0"/>', f'z="{-4 + idx * 0.2}" x="{-0.1 + idx * 0.02}"/>')
        # template = template.replace('z="-4" x="0"/>', f'z="{-4 + idx * 0.1}" x="{0}"/>')
        # template = template.replace('z="-4" x="0"/>', f'z="{-4 + idx * 0.1}" x="{0}"/>')


        with open('./template_new.xml', 'w+') as f2:
            f2.write(template)

    print(f'Rendering frame {idx}...', end=' ')
    r = subprocess.run([ lightwave_path, './template_new.xml' ], capture_output=True)
    # print(r.stdout)
    # print(r.stderr)
    if r.returncode == 0:
        print("Done.")
    else:
        print("Failed!")

def get_bb(filename):
    data = vdb.read(filename, "density")
    data2 = vdb.read(filename, "flames")


    bb = data.evalActiveVoxelBoundingBox()
    bb2 = data2.evalActiveVoxelBoundingBox()

    bb_min = (min(bb[0][0], bb2[0][0]), min(bb[0][1], bb2[0][1]), min(bb[0][2], bb2[0][2]))
    bb_max = (max(bb[1][0], bb2[1][0]), max(bb[1][1], bb2[1][1]), max(bb[1][2], bb2[1][2]))
    
    return bb_min, bb_max

bb_min = (10000, 10000, 10000)
bb_max = (0, 0, 0)
for i in range(0, 120):
    input_name = f"./tests/volumes/smallCampfireVDB/smallCampfire_{i:04}.vdb"
    cmin, cmax = get_bb(input_name)
    bb_min = (min(bb_min[0], cmin[0]), min(bb_min[1], cmin[1]), min(bb_min[2], cmin[2]))
    bb_max = (max(bb_max[0], cmax[0]), max(bb_max[1], cmax[1]), max(bb_max[2], cmax[2]))

print(bb_min)
print(bb_max)

for i in range(0, 120):
    render(i)

