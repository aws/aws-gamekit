# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse
from http.server import executable
import logging
import os
import pathlib
import shutil

import sys
# add parent directory with common methods to python module search path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from common import run_and_log, set_env_path

logging.basicConfig(level=logging.INFO)

def add_subparser(parser):
    parser.add_argument('--shared', default=False, action='store_true', help="Build as shared library (for Unity on Android)")
    parser.add_argument('--no-shared', dest='shared', action='store_false', help="Build as static library (for Unreal on Android)")

def build(args):
    # configure dependency paths
    aws_sdk = set_env_path("GAMEKIT_ANDROID_AWSSDK_PATH",
                           "Path to AWSSDK compiled for android (ex: D:\development\AWSSDK_android): ")
    boost = set_env_path("GAMEKIT_ANDROID_BOOST_PATH",
                         "Path to Boost compiled for android (ex: D:\development\\boost_1_76_0_android): ")
    yaml_cpp = set_env_path("GAMEKIT_ANDROID_YAMLCPP_PATH",
                            "Path to yaml-cpp compiled for android (ex: D:\development\yaml-cpp\\build_android): ")
    gtest = set_env_path("GAMEKIT_ANDROID_GTEST_PATH", "Path to googletest (ex: D:\development\googletest\\build): ")

    # make sure we're in the root of the repository
    script_path = pathlib.Path(__file__).absolute()
    repository_root = script_path.parents[2]
    scripts = repository_root / "scripts" / "Android"
    os.chdir(repository_root)

    build_dir = repository_root / "build_android"
    shutil.rmtree(build_dir, ignore_errors=True)
    build_dir.mkdir()

    os.chdir(build_dir)
    cache_file = repository_root / "CMakeCache.txt"
    if cache_file.exists():
        os.remove(repository_root / "CMakeCache.txt")

    dependencies = [aws_sdk, boost, yaml_cpp, gtest]
    shared_build = 'ON' if args.shared else 'OFF'
    stl_type = 'c++_shared' if args.shared else 'c++_static'
    regenerate_project = [str(scripts / "regenerate_android_projects.bat")] + dependencies + [str(repository_root),
                                                                                              args.type] + [shared_build] + [stl_type]
    cmake_build = ["cmake", "--build", ".", "--target", "install"]
    run_and_log(regenerate_project)
    run_and_log(cmake_build)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Builds Android binaries for AWS GameKit Cpp.")
    parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for GKCpp, Debug or Release.")
    parser.add_argument('--shared', action=argparse.BooleanOptionalAction, help="Build as shared library (for Unity on Android)")
    args = parser.parse_args()

    build(args)
