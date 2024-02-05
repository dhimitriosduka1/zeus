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
print(root_path)
build_path = os.path.join(root_path, "build")
with open(os.path.join(root_path, "CMakeLists.txt")) as f:
    binary_name = re.search(r"set\s*\(MY_TARGET_NAME\s+([^)\s]+)\s*\)", f.read(), re.IGNORECASE)[1]
    lightwave_path = os.path.join(build_path, binary_name)

def render(idx):
    with open('./template.xml', 'rt') as f:
        template = f.read()
        template = template.replace('x="-0.2" y="0.0" z="1.5"', f'x="{-0.2}" y="{0.0}" z="{1.5 - idx * 0.15}"')
        # Animation
        template = template.replace('volumes0', f'volumes_new{idx}')
        
        

        


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

for i in range(0, 40):
    render(i)

