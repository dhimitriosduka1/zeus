import pyopenvdb as vdb
import numpy as np

# Very slow and stupid way to convert VDB grid to plaintext file (aka worst volumetric format ever existed)
def convert(filename, outputname, outputname2):
    data = vdb.read(filename, "density")
    data2 = vdb.read(filename, "flames")

    bb = data.evalActiveVoxelBoundingBox()
    bb2 = data2.evalActiveVoxelBoundingBox()
    min_bb = (min(bb[0][0], bb2[0][0]), min(bb[0][1], bb2[0][1]), min(bb[0][2], bb2[0][2]))
    res = (max(bb[1][0], bb2[1][0]) + 1 - min_bb[0], max(bb[1][1], bb2[1][1]) + 1 - min_bb[1], max(bb[1][2], bb2[1][2]) + 1 - min_bb[2])
    # res = (220, 220, 220)
    grid_list = np.empty((res[0] * res[1] * res[2] + 3), 'float32')

    grid_list2 = np.empty((res[0] * res[1] * res[2] + 3), 'float32')
    

    print('loading grid...', filename, f'{res[0]} x {res[1]} x {res[2]}')
    max_v = 0
    for item in data.iterOnValues():
        if (item.min[0] < 0) or (item.min[1] < 0) or (item.min[2] < 0):
            break
        item_pos = item.min
        item_pos = (item_pos[0] - min_bb[0], res[1] - 1 - (item_pos[1] - min_bb[1]), item_pos[2] - min_bb[2])
        item_idx = (item_pos[2] * res[0] * res[1]) + (item_pos[1] * res[0]) + item_pos[0]
        grid_list[item_idx + 3] = item.value
        if item.value > max_v:
            max_v = item.value

    grid_list /= max_v
    grid_list[0] = res[0]
    grid_list[1] = res[1]
    grid_list[2] = res[2]
    max_v = 0
    for item in data2.iterOnValues():
        item_pos = item.min
        item_pos = (item_pos[0] - min_bb[0], res[1] - 1 - (item_pos[1] - min_bb[1]), item_pos[2] - min_bb[2])
        item_idx = (item_pos[2] * res[0] * res[1]) + (item_pos[1] * res[0]) + item_pos[0]
        grid_list2[item_idx + 3] = item.value
        if item.value > max_v:
            max_v = item.value
    grid_list2 /= max_v
    grid_list2[0] = res[0]
    grid_list2[1] = res[1]
    grid_list2[2] = res[2]
    
    print("writing grid...", outputname)
    with open(outputname, 'wb') as f:
        grid_list.tofile(f)
    with open(outputname2, 'wb') as f:
        grid_list2.tofile(f)
    # with open(outputname, "w+") as f:
    #     f.write(f'{res[0]} {res[1]} {res[2]}\n')
    #     for value in grid_list:
    #         f.write(f'{(value / max_v):.8f} ')
    print('done.')

convert("./tests/volumes/ground_explosion_0075.vdb", "./tests/volumes/density.vol", "./tests/volumes/flames.vol")