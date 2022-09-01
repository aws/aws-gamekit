# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse
from fnmatch import fnmatch
import logging
import os
import pathlib
import re

import sys
# add parent directory with common methods to python module search path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from common import copy_libs, REPOSITORY_ROOT

REQUIRED_ANDROID_LIBS = [
    r"libaws-cpp-sdk-apigateway\.so",
    r"libaws-cpp-sdk-cloudformation\.so",
    r"libaws-cpp-sdk-cognito-idp\.so",
    r"libaws-cpp-sdk-core\.so",
    r"libaws-cpp-sdk-lambda\.so",
    r"libaws-cpp-sdk-s3\.so",
    r"libaws-cpp-sdk-secretsmanager\.so",
    r"libaws-cpp-sdk-ssm\.so",
    r"libaws-cpp-sdk-sts\.so",
    r"libaws-gamekit-achievements\.so",
    r"libaws-gamekit-authentication\.so",
    r"libaws-gamekit-core\.so",
    r"libaws-gamekit-game-saving\.so",
    r"libaws-gamekit-identity\.so",
    r"libaws-gamekit-user-gameplay-data\.so",
    r"libboost_filesystem.*\.so",
    r"libboost_iostreams.*\.so",
    r"libboost_regex.*\.so",
    r"libc\+\+_shared\.so",
    r"libyaml-cpp\.so"
]

def refresh(type, plugin_path):
    # Unity only supports targeting one set of binaries, so refreshing with a different BuildType
    # will overwrite older BuildType binaries in the plugin.
    destination = pathlib.Path(plugin_path) / "Plugins" / "Android" / "libs" / "armeabi-v7a"
    bin = pathlib.Path(REPOSITORY_ROOT, "install", type, "bin")
    libs = pathlib.Path(REPOSITORY_ROOT, "install", type, "lib")

    copy_libs(".so", destination, binary_dir=bin)
    copy_libs(".so", destination, binary_dir=libs)


    missing_libs = find_missing_libs(os.listdir(destination), ".so")

    if len(missing_libs) > 0: 
        logging.warn("""\nMISSING libraries: [ {missing_libs} ], \n 
        try calling the build script with the '--shared' option or the refresh script with '--build'. \n\n""".format(missing_libs=", ".join(missing_libs)))

def find_missing_libs(present_libs, extension):
    present_libs = [ file for file in present_libs if file.endswith(extension)]
    present_libs_string = " ".join(present_libs)
    missing_libs = []
    for lib_name in REQUIRED_ANDROID_LIBS:
        if re.search(lib_name, present_libs_string) == None:
            missing_libs.append(lib_name.replace('\\', ''))
    return missing_libs


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Copies library files over to the target plugin.")
    parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for libs being copied over.")
    parser.add_argument("--unity_plugin_path", required=True, help="Path to AWS GameKit Plugin for Unity e.g. [unity_package]/Packages/com.amazonaws.gamekit")
    args = parser.parse_args()

    refresh(args.type, args.unity_plugin_path)