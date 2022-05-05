# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
import os
import pathlib
import subprocess

import sys
# add parent directory with common methods to python module search path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from common import run_and_log, set_env_path

logging.basicConfig(level=logging.INFO)

def add_subparser(parser):
    parser.add_argument("--test", action="store_true", default=False, help="Whether it should run unit tests.")

def build(args):
    aws_sdk = set_env_path("GAMEKIT_AWSSDK_PATH_mac", "Path to AWSSDK (ex: ~/development/AWSSDK_mac): ")
    boost = set_env_path("GAMEKIT_BOOST_PATH_mac", "Path to Boost (ex: ~/development/boost_1_76_0): ")
    yaml_cpp = set_env_path("GAMEKIT_YAMLCPP_PATH_mac", "Path to yaml-cpp (ex: ~/development/yaml-cpp): ")
    gtest = set_env_path("GAMEKIT_GTEST_PATH_mac", "Path to googletest (ex: ~/development/googletest): ")

    # make sure we're in the root of the repository
    script_path = pathlib.Path(__file__).absolute()
    repository_root = script_path.parents[2]
    scripts = repository_root / "scripts" / "Mac"
    os.chdir(repository_root)

    cache_file = repository_root / "CMakeCache.txt"
    if cache_file.exists():
        os.remove(repository_root / "CMakeCache.txt")

    dependencies = [aws_sdk, boost, yaml_cpp, gtest]

    run_and_log([str(scripts / "regenerate_projects_mac.sh")] + dependencies + [str(repository_root), args.type])
    run_and_log(["xcodebuild", "-parallelizeTargets", "-configuration", args.type, "-target", "ALL_BUILD"])
    run_and_log(["xcodebuild", "-parallelizeTargets", "-target", "install"])

    if args.test:
        test_dir = pathlib.Path("tests") / args.type
        os.chdir(test_dir)
        subprocess.run("./aws-gamekit-cpp-tests", shell=True)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Builds and tests MacOS binaries for AWS GameKit Cpp.")
    parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for GKCpp, Debug or Release.")
    parser.add_argument("--test", action="store_true", default=False, help="Whether it should run unit tests.")
    add_subparser(parser)
    args = parser.parse_args()

    build(args)


