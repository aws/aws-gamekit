# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
import os
import pathlib

import sys
# add parent directory with common methods to python module search path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from common import run_and_log, set_env_path

logging.basicConfig(level=logging.INFO)

def add_subparser(parser):
    parser.add_argument("--vs_type", choices=["Professional", "BuildTools", "Community"], default="Professional", required=False)
    parser.add_argument("--vs_version", default="2019", required=False, help="""Which year of visual studio is installed in this environment. 
        2019 is the supported version for building this project, adjustments may need to be made for other versions.""")
    parser.add_argument("--clean", action="store_true", default=False, help="Whether it should clean ALL_BUILD before compiling.")
    parser.add_argument("--test", action="store_true", default=False, help="Whether it should run unit tests.")
    parser.add_argument("--ms_build_path", required=False, help="Path to MSBuild.exe, use instead of vs_verion + vs_type if it's in an unusual location.")

def build(args):
    aws_sdk = set_env_path("GAMEKIT_AWSSDK_PATH", "Path to AWSSDK (ex: D:\development\AWSSDK): ")
    boost = set_env_path("GAMEKIT_BOOST_PATH", "Path to Boost (ex: D:\development\\boost_1_76_0): ")
    yaml_cpp = set_env_path("GAMEKIT_YAMLCPP_PATH", "Path to yaml-cpp (ex: D:\development\yaml-cpp): ")
    gtest = set_env_path("GAMEKIT_GTEST_PATH", "Path to googletest (ex: D:\development\googletest): ")
    pybind = set_env_path("GAMEKIT_PYBIND_PATH", "Path to pybind (ex: D:\development\pybind11): ")

    if args.ms_build_path:
        MS_BUILD = pathlib.Path(args.ms_build_path)
    else:
        MS_BUILD = pathlib.Path("C:\\") / "Program Files (x86)" / "Microsoft Visual Studio" / args.vs_version / args.vs_type / "MSBuild" / "Current" / "Bin" / "MSBuild.exe"

    if not MS_BUILD.is_file():
        logging.error(f"MSBBuild.exe does not exist at path: {MS_BUILD}")
        logging.error(f"Try setting the --ms_build_path argument to point to a valid MSBuild.exe executable.")
        sys.exit(1)

    # make sure we're in the root of the repository
    script_path = pathlib.Path(__file__).absolute()
    repository_root = script_path.parents[2]
    scripts = repository_root / "scripts" / "Win64"
    os.chdir(repository_root)

    cache_file = repository_root / "CMakeCache.txt"
    if cache_file.exists():
        os.remove(repository_root / "CMakeCache.txt")

    dependencies = [aws_sdk, boost, yaml_cpp, gtest, pybind]

    run_and_log([str(scripts / "regenerate_projects.bat")] + dependencies + [str(repository_root), args.type])
    run_and_log([str(MS_BUILD), "ALL_BUILD.vcxproj", f"-p:Configuration={args.type}", "-m", "-t:Clean" if args.clean else ""])
    run_and_log([str(MS_BUILD), "INSTALL.vcxproj", f"-p:Configuration={args.type}", "-m"])

    if args.test:
        test_dir = pathlib.Path("tests") / args.type
        os.chdir(test_dir)
        run_and_log(["aws-gamekit-cpp-tests.exe"])

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Builds and tests AWS GameKit cpp sdk.")
    parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for GKCpp, Debug or Release.")
    add_subparser(parser)
    args = parser.parse_args()

    build(args)


