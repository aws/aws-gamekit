# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import pathlib

import sys
# add parent directory with common methods to python module search path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from common import copy_libs, REPOSITORY_ROOT

def refresh(type, plugin_path):
    # Unity only supports targeting one set of binaries, so refreshing with a different BuildType
    # will overwrite older BuildType binaries in the plugin.
    destination = pathlib.Path(plugin_path) / "Plugins" / "Windows" / "x64"
    libs = pathlib.Path(REPOSITORY_ROOT, "install", type, "bin")
    copy_libs(".dll", destination, binary_dir=libs)
    
    if type == "Debug":
        copy_libs(".pdb", destination, binary_dir=libs)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Copies library files over to the target plugin.")
    parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for libs being copied over.")
    parser.add_argument("--unity_plugin_path", required=True, help="Path to AWs GameKit Plugin for Unity e.g. [unity_package]/Packages/com.amazonaws.gamekit")
    args = parser.parse_args()

    refresh(args.type, args.unity_plugin_path)
