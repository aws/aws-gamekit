# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import pathlib
import re
import subprocess

import sys
# add parent directory with common methods to python module search path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from common import code_sign

# Use this script so binaries can be distributed and packaged on other computers
# Not idempotent, only run once after recompiling for a specific BuildType (Debug/Release)

def fix_rpath(target_lib):
    otool_cmd = f"otool -l {target_lib}| grep RPATH -A2"
    output = subprocess.check_output(otool_cmd, shell=True)

    rpaths = []
    for line in output.decode('utf-8').split("\n"):
        # look for line that matches: path <absolute_path> (offset <number>)
        pattern = "\s*path\s+(\S*)\s+\(offset.*\)\s*"
        match = re.match(pattern, line)
        if match:
            rpath = match.group(1)
            print(f"Removing rpath: {rpath} from {target_lib.name}")
            old_rpath = f"\\{rpath}" if "origin" in rpath.lower() else rpath
            subprocess.run(f"install_name_tool -delete_rpath {old_rpath} {target_lib}", shell=True)

    # add @loader_path as the only rpath, dylibs assume other libs are adjacent to it after this.
    subprocess.run(f'install_name_tool -add_rpath @loader_path {target_lib}', shell=True)

def fix_symlinks(target_lib):
    # aws-cpp-sdk makes a lot of symlinks when you compile it, change rpaths to point to the target of the symlink.
    symlink_dict = {
        "libaws-c-auth.dylib": "libaws-c-auth.1.0.0.dylib",
        "libaws-c-cal.dylib": "libaws-c-cal.1.0.0.dylib",
        "libaws-c-common.1.dylib": "libaws-c-common.1.0.0.dylib",
        "libaws-c-common.dylib": "libaws-c-common.1.dylib",
        "libaws-c-compression.dylib": "libaws-c-compression.1.0.0.dylib",
        "libaws-c-event-stream.dylib": "libaws-c-event-stream.1.0.0.dylib",
        "libaws-c-http.dylib":  "libaws-c-http.1.0.0.dylib",
        "libaws-c-io.dylib": "libaws-c-io.1.0.0.dylib",
        "libaws-c-mqtt.dylib": "libaws-c-mqtt.1.0.0.dylib",
        "libaws-c-s3.0unstable.dylib": "libaws-c-s3.1.0.0.dylib",
        "libaws-c-s3.dylib": "libaws-c-s3.0unstable.dylib",
        "libaws-c-sdkutils.dylib": "libaws-c-sdkutils.1.0.0.dylib",
        "libaws-checksums.dylib": "libaws-checksums.1.0.0.dylib",
    }

    for link, target in symlink_dict.items():
        # Change every dylib link from link to the links target.
        cmd = f"install_name_tool -change @rpath/{link} @rpath/{target} {target_lib}"
        try:
            subprocess.run(cmd, shell=True)
        except:
            # Errors are when this lib doesn't depend on target, so ignore.
            pass

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for AWS GameKit C++, Debug or Release.")
    parser.add_argument("--certificate_name", default="", help="Common name of the code signing certificate.")
    parser.add_argument("--binaries_dir", help="Directory that contains binaries to fix.")
    args = parser.parse_args()

    script_path = pathlib.Path(__file__).absolute().parent
    repository_root = script_path.parents[1]

    binaries_dir = pathlib.Path(args.binaries_dir)
    for f in binaries_dir.iterdir():
        if not f.is_dir() and str(f.name).startswith("libaws-") and str(f.name).endswith(".dylib"):
            fix_rpath(f)
            fix_symlinks(f)

            if args.certificate_name:
                code_sign(f, args.certificate_name)
