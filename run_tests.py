#! /usr/bin/env python3

import glob
import os
import sys
import subprocess
import time
import urllib.parse
import urllib.request
import re
import argparse

parser = argparse.ArgumentParser(description='Test runner for lightwave')
parser.add_argument('filenames', metavar='tests', type=str, nargs='*', default=["*/*.xml"],
                    help='which test files to run')
parser.add_argument('--unsafe', dest='unsafe_tests', action='store_true',
                    help='include unsafe tests')
parser.add_argument('--disable-build', dest='disable_build', action='store_true',
                    help='do not build lightwave before running the tests')

args = parser.parse_args()
root_path = os.path.relpath(os.path.dirname(__file__), os.path.curdir)
build_path = os.path.join(root_path, "build")

try:
    with open(os.path.join(root_path, "CMakeLists.txt")) as f:
        binary_name = re.search(r"set\s*\(MY_TARGET_NAME\s+([^)\s]+)\s*\)", f.read(), re.IGNORECASE)[1]
        lightwave_path = os.path.join(build_path, binary_name)
except:
    print(f"Could not determine the name of your renderer")
    exit(1)

if not os.path.isdir(build_path):
    print(f"Could not find build path, have you already configured the project using CMake?")
    exit(1)

if not args.disable_build:
    #os.makedirs(build_path, exist_ok=True)
    #subprocess.run(["cmake",
    #    "-S", root_path,
    #    "-B", build_path
    #])
    try:
        build = subprocess.run(["cmake",
            "--build", build_path,
            "--parallel"
        ])
        if build.returncode != 0:
            exit(build.returncode)
    except:
        print("Could not build lightwave for you, please make sure 'cmake' is in your PATH environment variable.")

tests = []
for filename in args.filenames:
    if os.path.isfile(filename):
        tests += [filename]
        continue
    
    path = os.path.join(root_path, "tests", filename)
    if os.path.isfile(path):
        tests += [path]
    elif os.path.isdir(path):
        tests += glob.glob(os.path.join(path, "*.xml"))
    elif os.path.isfile(path + ".xml"):
        tests += [path + ".xml"]
    else:
        tests += glob.glob(path)
tests = list(set(tests))

if len(tests) == 0:
    print("No tests match your input")
    exit(0)

start_time = time.time()
passed_count = 0
total_count = 0

print()
for test in sorted(tests):
    test_name = test.replace("\\", "/").split("/")[-2:]
    test_name = "/".join(test_name).split(".")[0]
    total_count += 1
    
    print(f"\033[90m› {test_name}\033[0m", end="", flush=True)
    test_start = time.time()
    r = subprocess.run([ lightwave_path, test ], capture_output=True)
    elapsed_seconds = time.time() - test_start

    print("\33[02K\r", end="", flush=True)
    if r.returncode == 0:
        print(f"\033[92m✓ {test_name} passed\033[0m ({elapsed_seconds:.2f}s)")
        passed_count += 1
        continue

    try:
        error = r.stderr.decode()
    except:
        # Windows still hasn't gotten Unicode right
        error = r.stderr.decode("utf-16")
    
    print(f"\033[91m⨯ {test_name} failed\033[0m")
    print("\n".join(error.split("\n")[-6:-1]))
    print()
    all_passed = False

all_passed = passed_count == total_count
elapsed_seconds = time.time() - start_time
message = "\033[92mAll tests passed\033[0m" if all_passed else "\033[91mSome tests failed\033[0m"
print()
print(f"{message} ({passed_count}/{total_count} passed, {elapsed_seconds:.2f}s)")
print(f"\033[90mTest coverage: {100 * (passed_count / total_count):.1f}%\033[0m")
print()

print("Note:\033[90m")
print("  Our tests are based on simple error metrics and only meant to guide you.")
print("  Passing all tests is not a guarantee that you will receive full points!")
print("\033[0m")
print()

if args.unsafe_tests:
    print("DISCLAIMER:\033[90m")
    print("  Our renderer name tests relies on the OpenAI GPT API to grade your name choice with imaginary points.")
    print("  These points do not affect your grade (although we may award a prize for the highest score).")
    print("  We are not liable for the output produced by OpenAI, and by running this test you agree that your renderer name is sent to OpenAI for processing.")
    print("\033[0m")
    
    try:
        contents = urllib.request.urlopen("https://lightgrade.besonders.cool/api/test?name=" + urllib.parse.quote_plus(binary_name)).read().decode('utf-8')
        if (v := re.search(r"^(\d+ point(s|)).\s+", contents)):
            name_grade = f"{v[1]} for \033[3m{binary_name}\033[0m"
            contents = contents[len(v[0]):]
        else:
            name_grade = f"\033[3m{binary_name}\033[0m"

        print(f"\033[92m✓ practical_1/renderer_name\033[0m ({name_grade})")
        print("\033[90m  " + contents + "\033[0m")
        print()
    except:
        # it's fine if this test does not pass :-)
        pass

sys.exit(0 if all_passed else 1)
