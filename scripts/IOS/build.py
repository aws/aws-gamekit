# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse
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
    parser.add_argument("--clean", action="store_true", default=False, help="Whether it should clean build directory before compiling.")

def build(args):
    aws_sdk = set_env_path("GAMEKIT_AWSSDK_PATH_ios", "Path to AWSSDK (ex: ~/development/AWSSDK_ios): ")
    boost = set_env_path("GAMEKIT_BOOST_PATH_mac", "Path to Boost (ex: ~/development/boost_1_76_0): ")
    yaml_cpp = set_env_path("GAMEKIT_YAMLCPP_PATH_ios", "Path to yaml-cpp (ex: ~/development/yaml-cpp): ")
    curl = set_env_path("GAMEKIT_CURL_PATH_ios", "Path to curl (ex: ~/development/Build-OpenSSL-cURL): ")

    # make sure we're in the root of the repository
    script_path = pathlib.Path(__file__).absolute()
    repository_root = script_path.parents[2]
    scripts = repository_root / "scripts" / "IOS"
    os.chdir(repository_root)

    cache_file = repository_root / "CMakeCache.txt"
    if cache_file.exists():
        os.remove(repository_root / "CMakeCache.txt")

    build_dir = repository_root / "build_ios"
    if build_dir.exists() and args.clean:
        shutil.rmtree(build_dir)
    
    build_dir.mkdir(exist_ok=True)
    os.chdir(build_dir)

    dependencies = [aws_sdk, boost, yaml_cpp, curl, str(repository_root), args.type]

    run_and_log([str(scripts / "regenerate_projects_ios.sh")] + dependencies)
    run_and_log(["xcodebuild", "-parallelizeTargets", "-configuration", args.type, "-target", "ALL_BUILD"])

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Builds iOS binaries for AWS GameKit Cpp.")
    parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for GKCpp, Debug or Release.")
    add_subparser(parser)
    args = parser.parse_args()

    build(args)