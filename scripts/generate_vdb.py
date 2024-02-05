import numpy as np

# Very slow and stupid way to convert VDB grid
def generate(res, outputname):
    grid_list = np.empty((res[0] * res[1] * res[2] + 3), 'float32')
    grid_list[0] = res[0]
    grid_list[1] = res[1]
    grid_list[2] = res[2]
    
    print('creating grid...', outputname, f'{res[0]} x {res[1]} x {res[2]}')
    max_v = 0
    for x in range(res[0]):
        for y in range(res[1]):
            for z in range(res[2]):
                item_pos = (x, y, z)
                item_idx = (item_pos[2] * res[0] * res[1]) + (item_pos[1] * res[0]) + item_pos[0]
                value = 0 if z > 0 else 1
                grid_list[item_idx + 3] = value
                if value > max_v:
                    max_v = value

    print("writing grid...", outputname)
    with open(outputname, 'wb') as f:
        grid_list.tofile(f)
    print('done.')

# convert("./tests/volumes/cloud.vdb", "./tests/volumes/cloud.vol", "density")
generate((2,2,2), "./test.vol")
