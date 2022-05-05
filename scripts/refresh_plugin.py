# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

import argparse
import logging
import os
import pathlib
import shutil
import sys

import common

import Android.refresh_unreal as android_unreal
import Win64.refresh_unreal as windows_unreal
import Win64.refresh_unity as windows_unity 
import Mac.refresh_unreal as mac_unreal
import IOS.refresh_unreal as ios_unreal

from aws_gamekit_cpp_build import build_libs

def copy_unreal_headers(plugin_path, generate_error_codes=True):
    plugin_path = pathlib.Path(plugin_path)
    base_header_destination = plugin_path / "Libraries" / "include" / "aws" / "gamekit"
    root = pathlib.Path(__file__).absolute().parents[1]
    targets = {
        ("identity",):["exports.h", "gamekit_identity_models.h"],
        ("authentication",): ["exports.h"],
        ("achievements",): ["exports.h", "gamekit_achievements_models.h"],
        ("game-saving",): ["exports.h", "gamekit_game_saving_models.h"],
        ("user-gameplay-data",): ["exports.h", "gamekit_user_gameplay_data_models.h"],
        ("core",): ["api.h", "enums.h", "errors.h", "exports.h", "logging.h", "feature_resources_callback.h"],
        ("core", "model"): ["account_credentials.h", "account_info.h", "config_consts.h", "resource_environment.h", "template_consts.h"],
        ("core", "utils"): ["encoding_utils.h", "gamekit_httpclient_callbacks.h", "ticker.h"]
    }

    for key, val in targets.items():
        destination = base_header_destination
        # for target keys with multiple layers concatenate onto path
        for k in key:
            destination = destination / k
        destination.mkdir(parents=True, exist_ok=True)

        for header_file in val:
            source = root / f"aws-gamekit-{key[0]}" / "include" / "aws" / "gamekit"
            for k in key:
                source =  source / k
            source = source / header_file

            file_destination = destination / header_file
            shutil.copy2(str(source), str(file_destination))

    if generate_error_codes:
        common.run_and_log(["python", str(plugin_path / "generate_error_code_blueprint.py"), str(plugin_path.parent)])

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Copies library files over to the target plugin.")

    parser.add_argument("--build", action="store_true", default=False, help="Whether the plugin should be rebuilt for these platforms as well.")
    parser.add_argument("--platform", required=True, default="Windows", choices=["Windows", "Android", "Mac", "iOS"], help="Platform target for libs being copied over.")
    parser.add_argument("--no_code_generation", action="store_true", help="Whether to call code generation scripts for the AWS GameKit unreal plugin.")

    # Can't add a second subparser with it's own key word args so have certificate_name which codesigns mac/ios binaries as always optional
    parser.add_argument("--certificate_name", help="Apple Developer ID Application certificate for code signing.")
    parser.add_argument("type", choices=["Debug", "Release"], help="Compile type for libs being copied over.")

    engine_subparser = parser.add_subparsers(help="Target platform type.", dest="engine", required=True)

    unreal_parser = engine_subparser.add_parser("Unreal")
    unreal_parser.add_argument("--unreal_plugin_path", required=True, help="Path to AWS GameKit Plugin for Unreal e.g. [unreal_project_path]/Plugins/AwsGameKit")

    unity_parser = engine_subparser.add_parser("Unity")
    unity_parser.add_argument("--unity_plugin_path", required=True, help="Path to AWS GameKit Plugin for Unity Unity e.g. [unity_package]/Assets/AWS GameKit")

    args = parser.parse_args()

    if args.build:
        # Will need to be altered if build tools installed in non-standard locations
        common.run_and_log(["python", "aws_gamekit_cpp_build.py", args.platform, args.type])

    if args.engine == "Unreal":
        if not pathlib.Path(args.unreal_plugin_path).exists():
            logging.error(f"Unreal plugin path does not exist: {args.unreal_plugin_path}")
            sys.exit(1)

        copy_unreal_headers(args.unreal_plugin_path, not args.no_code_generation)

        if "Windows" == args.platform:
            windows_unreal.refresh(args.type, args.unreal_plugin_path)
        if "Android" == args.platform:
            android_unreal.refresh(args.type, args.unreal_plugin_path)
        if "Mac" == args.platform:
            mac_unreal.refresh(args.type, args.unreal_plugin_path, args.certificate_name)
        if "iOS" == args.platform:
            ios_unreal.refresh(args.type, args.unreal_plugin_path, args.certificate_name)
    elif args.engine == "Unity":
        if "Windows" == args.platform:
            windows_unity.refresh(args.type, args.unity_plugin_path)
        if "Android" == args.platform:
            # TODO
            pass
        if "Mac" == args.platform:
            # TODO
            pass
        if "iOS" == args.platform:
            # TODO
            pass