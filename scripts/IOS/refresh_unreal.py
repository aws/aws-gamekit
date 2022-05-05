# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import pathlib

import sys
# add parent directory with common methods to python module search path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from common import set_env_path, copy_libs, copy_file, REPOSITORY_ROOT, FEATURES, code_sign

def refresh(type, plugin_path, certificate_name=None):
    destination = pathlib.Path(plugin_path).expanduser() / "Libraries" / "IOS" / type

    aws_sdk = set_env_path("GAMEKIT_AWSSDK_PATH_ios", "Path to AWSSDK (ex: ~/development/AWSSDK_ios): ")
    boost = set_env_path("GAMEKIT_BOOST_PATH_ios", "Path to Boost (ex: ~/development/ios-boost): ")
    yaml_cpp = set_env_path("GAMEKIT_YAMLCPP_PATH_ios", "Path to yaml-cpp (ex: ~/development/yaml-cpp): ")
    curl = set_env_path("GAMEKIT_CURL_PATH_ios", "Path to curl (ex: ~/development/Build-OpenSSL-cURL): ")

    copy_libs(".a", destination, binary_dir=pathlib.Path(aws_sdk, "install", "arm64", "ios", type, "lib"))
    copy_libs(".a", destination / "boost", binary_dir=pathlib.Path(boost, "ios", "build", "arm64"))

    copy_file(pathlib.Path(yaml_cpp, "build_ios", "install", "arm64", type, "lib", "libyaml-cppd.a"), destination / "yaml-cpp")
    copy_file(pathlib.Path(curl, "nghttp2", "iOS", "arm64", "lib", "libnghttp2.a"), destination / "nghttp2")
    copy_file(pathlib.Path(curl, "curl", "lib", "libcurl.a"), destination / "curl")
    copy_file(pathlib.Path(curl, "openssl", "iOS", "lib", "libssl.a"), destination / "openssl")
    copy_file(pathlib.Path(curl, "openssl", "iOS", "lib", "libcrypto.a"), destination / "openssl")

    # Only curl and openssl libs need to be codesigned
    if certificate_name:
        code_sign(str(pathlib.Path(destination, "curl", "libcurl.a")), certificate_name)
        code_sign(str(destination / "openssl" / "libssl.a"), certificate_name)
        code_sign(str(destination / "openssl" / "libcrypto.a"), certificate_name)

    for f in FEATURES:
        copy_libs(".a", destination, binary_dir=pathlib.Path(REPOSITORY_ROOT, "build_ios", f"aws-gamekit-{f}", f"{type}-iphoneos"))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Copies library files over to the target plugin.")
    parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for libs being copied over.")
    parser.add_argument("--unreal_plugin_path", required=True, help="Path to AWS GameKit Plugin for Unreal e.g. [unreal_project_path]/Plugins/AwsGameKit")
    parser.add_argument("--certificate_name", help="Apple Developer ID Application certificate for code signing.")
    args = parser.parse_args()

    refresh(args.type, args.unreal_plugin_path, args.certificate_name)