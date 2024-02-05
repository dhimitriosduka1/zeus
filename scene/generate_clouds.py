import random

template = """<instance>
            <shape type="mesh" filename="./meshes/cube.ply" smooth="false" id="cloudmesh"/>
            <volume type="grid" filename="../tests/volumes/cloud.vol" multiplier="35.5" id="cloudvolume"/>
            <bsdf type="volume" absorption="4.0" light="1.0" phase="0.0" id="cloudshader"/>
            <transform>
                <scale x="-405" y="94" z="403"/>
                <scale value="0.04"/>
                <rotate axis="1,0,0" angle="180.0"/>
                <rotate axis="0,1,0" angle="25.0"/>
                <translate x="7.0" y="8.25" z="21.0"/>
            </transform>
        </instance>"""

result = ""

z_steps = 35
z_max = 950
z_step_size = z_max / z_steps
width_max = 420
width_min = 30
amount_min = 5
amount_max = 12
size_min = 0.035
size_max = 0.06

def lerp(a, b, t):
    return a * (1 - t) + b * t

for z in range(z_steps):
    z_pos = z * z_step_size
    z_t = float(z) / (z_steps - 1)
    amount = int(lerp(amount_min, amount_max, z_t))
    width = lerp(width_min, width_max, z_t)

    for i in range(amount):
        new = f"""<instance>
                <ref id="cloudmesh"/>
                <ref id="cloudvolume"/>
                <ref id="cloudshader"/>
                <transform>
                    <scale x="{405 if random.random() > 0.5 else -405}" y="94" z="{403 if random.random() > 0.5 else -403}"/>
                    <scale value="{random.uniform(size_min, size_max)}"/>
                    <rotate axis="1,0,0" angle="{180 + random.uniform(-10, 10)}"/>
                    <rotate axis="0,1,0" angle="{random.uniform(0, 360)}"/>
                    <translate x="{random.uniform(-width, width)}" y="{9.5 + random.uniform(0.0, 2.5) + lerp(0, 8.5, z_t)}" z="{z_pos + 30}"/>
                </transform>
            </instance>\n"""
    
        result += new

with open("clouds.txt", "w+") as f:
    f.write(result)