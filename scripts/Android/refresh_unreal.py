# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import pathlib

import sys
# add parent directory with common methods to python module search path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from common import copy_libs, REPOSITORY_ROOT

def add_parser_arguments(parser):
	parser.add_argument("--android_architecture", choices=["armv7", "arm64"], default="armv7", help="Architecture of Android libraries. Defaults to armv7")

def refresh(type, plugin_path, android_architecture):
	destination = pathlib.Path(plugin_path) / "Libraries" / "Android" / android_architecture / type
	bin = pathlib.Path(REPOSITORY_ROOT, "install", type, "bin")
	libs = pathlib.Path(REPOSITORY_ROOT, "install", type, "lib")

	copy_libs(".a", destination, binary_dir=bin)
	copy_libs(".a", destination, binary_dir=libs)

if __name__ == '__main__':
	parser = argparse.ArgumentParser(description="Copies library files over to the target plugin.")
	parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for libs being copied over.")
	parser.add_argument("--unreal_plugin_path", required=True, help="Path to AWS GameKit Plugin for Unreal e.g. [unreal_project_path]/Plugins/AwsGameKit")
	parser.add_argument("--android_architecture", choices=["armv7", "arm64"], default="armv7", help="Architecture of Android libraries. Defaults to armv7")
	args = parser.parse_args()

	refresh(args.type, args.unreal_plugin_path, args.android_architecture)