# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import pathlib

import sys
# add parent directory with common methods to python module search path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from common import run_and_log, copy_libs, FEATURES, REPOSITORY_ROOT

def refresh(type, plugin_path, certificate_name=None):
    destination = pathlib.Path(plugin_path).expanduser() / "Plugins" / "Mac"

    for feature in FEATURES:
        copy_libs(".dylib", destination, binary_dir=pathlib.Path(REPOSITORY_ROOT, f"aws-gamekit-{feature}", type))
    aws_sdk_libs = pathlib.Path(REPOSITORY_ROOT, "install", "x86_64", type, "bin")
    copy_libs(".dylib", destination, binary_dir=aws_sdk_libs)

    # Fix all rpaths and symlinks in the libs after they're consolidated in one directory in unity
    rpath_script = REPOSITORY_ROOT / "scripts" / "Mac" / "load_path_everything.py"
    cmd = ["python", str(rpath_script), type, "--binaries_dir", str(destination)]

    if certificate_name:
        cmd += ["--certificate_name", certificate_name]
        
    run_and_log(cmd)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Copies library files over to the target plugin.")
    parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for libs being copied over.")
    parser.add_argument("--unity_plugin_path", required=True, help="Path to AWs GameKit Plugin for Unity e.g. [unity_package]/Packages/com.amazonaws.gamekit")
    parser.add_argument("--certificate_name", help="Apple Developer ID Application certificate for code signing.")
    args = parser.parse_args()

    refresh(args.type, args.unity_plugin_path, args.certificate_name)
