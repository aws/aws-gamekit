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
    parser.add_argument("--enable_pybind", action="store_true", default=False, help="Whether to build the project for load testing.")

def build(args):
    aws_sdk = set_env_path("GAMEKIT_AWSSDK_PATH_mac", "Path to AWSSDK (ex: ~/development/AWSSDK_mac): ")
    boost = set_env_path("GAMEKIT_BOOST_PATH_mac", "Path to Boost (ex: ~/development/boost_1_76_0): ")
    yaml_cpp = set_env_path("GAMEKIT_YAMLCPP_PATH_mac", "Path to yaml-cpp (ex: ~/development/yaml-cpp): ")

    # make sure we're in the root of the repository
    script_path = pathlib.Path(__file__).absolute()
    repository_root = script_path.parents[2]
    scripts = repository_root / "scripts" / "Mac"
    os.chdir(repository_root)

    cache_file = repository_root / "CMakeCache.txt"
    if cache_file.exists():
        os.remove(repository_root / "CMakeCache.txt")

    cmake_cmd = (
        f'cmake -G "Xcode" -DCMAKE_BUILD_TYPE={args.type} -DBUILD_SHARED_LIBS=ON '
        f'-Daws-c-sdkutils_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-c-sdkutils/cmake '
        f'-Daws-c-http_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-c-http/cmake '
        f'-Daws-c-io_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-c-io/cmake '
        f'-Daws-c-common_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-c-common/cmake '
        f'-Daws-c-cal_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-c-cal/cmake '
        f'-Daws-c-compression_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-c-compression/cmake '
        f'-Daws-c-mqtt_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-c-mqtt/cmake '
        f'-Daws-c-auth_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-c-auth/cmake '
        f'-Daws-c-event-stream_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-c-event-stream/cmake '
        f'-Daws-checksums_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-checksums/cmake '
        f'-Daws-c-s3_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-c-s3/cmake '
        f'-DAWSSDK_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/cmake/AWSSDK '
        f'-Daws-cpp-sdk-core_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/cmake/aws-cpp-sdk-core '
        f'-Daws-crt-cpp_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-crt-cpp/cmake '
        f'-Daws-cpp-sdk-cloudformation_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/cmake/aws-cpp-sdk-cloudformation '
        f'-Daws-cpp-sdk-cognito-idp_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/cmake/aws-cpp-sdk-cognito-idp '
        f'-Daws-cpp-sdk-sts_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/cmake/aws-cpp-sdk-sts '
        f'-Daws-cpp-sdk-secretsmanager_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/cmake/aws-cpp-sdk-secretsmanager '
        f'-Daws-cpp-sdk-ssm_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/cmake/aws-cpp-sdk-ssm '
        f'-Daws-cpp-sdk-s3_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/cmake/aws-cpp-sdk-s3 '
        f'-Daws-cpp-sdk-lambda_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/cmake/aws-cpp-sdk-lambda '
        f'-Daws-cpp-sdk-apigateway_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/cmake/aws-cpp-sdk-apigateway '
        f'-Daws-c-sdkutils_DIR={aws_sdk}/install/x86_64/macos/{args.type}/lib/aws-c-sdkutils/cmake '
        f'-DBOOST_ROOT={boost} -DBOOST_LIBRARYDIR={boost}/stage/lib '
        f'-Dyaml-cpp_DIR={yaml_cpp}/build_mac/install/x86_64/{args.type}/share/cmake/yaml-cpp '
        '-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 '
        f'-DCMAKE_INSTALL_PREFIX=./install/x86_64/{args.type} '
        f'{repository_root} '
    )

    if args.test:
        gtest = set_env_path("GAMEKIT_GTEST_PATH_mac", "Path to googletest (ex: ~/development/googletest): ")
        cmake_cmd += (
            f'-DGTEST_LINKED_AS_SHARED_LIBRARY=1 '
            f'-DGTest_DIR={gtest}/build_mac/install/x86_64/lib/cmake/GTest '
            f'-DGTEST_SRC={gtest}/build_mac/install/x86_64/include '
        )

    if args.enable_pybind:
        pybind = set_env_path("GAMEKIT_PYBIND_PATH_mac", "Path to pybind (ex: ~/development/pybind11): ")
        cmake_cmd += f'-Dpybind11_DIR={pybind}/install/{args.type}/share/cmake/pybind11'
 
    run_and_log(cmake_cmd)
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


