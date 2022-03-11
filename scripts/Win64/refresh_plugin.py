# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
"""
Run `python refresh_plugin.py --help` to learn how to use this program.
"""
import argparse
from enum import Enum
from fnmatch import fnmatch
import logging
import os
from pathlib import Path
import shutil


class GameEngine(Enum):
    UNREAL = "Unreal"
    UNITY = "Unity"


class BuildType(Enum):
    DEBUG = "Debug"
    RELEASE = "Release"


class FilePatterns(Enum):
    DLL = '*.dll'
    PDB = '*.pdb'


def main():
    # Get command line arguments:
    args = get_arguments()
    destination_plugin_root = Path(args.plugin_full_path)
    game_engine = GameEngine(args.game_engine)
    build_type = BuildType(args.build_type)

    # Configure logging:
    configure_logging(args.quiet)

    # Determine paths:
    gamekit_sdk_root = get_absolute_path_of_this_python_script().joinpath('..', '..')
    source_binaries_dir = gamekit_sdk_root.joinpath('install', build_type.value, 'bin')
    destination_binaries_dir = get_destination_binaries_dir(destination_plugin_root, game_engine, build_type)

    # Copy binaries:
    clean_destination(destination_binaries_dir, destination_plugin_root, game_engine)
    copy_binaries_to_destination(source_binaries_dir, destination_binaries_dir, build_type)


def get_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Update an AWS GameKit plugin to use the latest binaries.",
        epilog="Example Usage:\n"
               "python refresh_plugin.py D:/workspace/GameKit/src/GameKitUnrealSandbox/AwsGameKitUnrealGame/Plugins Unreal Debug\n"
               "python refresh_plugin.py D:/workspace/GameKit/src/GameKitUnityPackage Unity Debug\n",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("plugin_full_path", type=str, help="Full path to the AWS GameKit plugin to update. See 'Example Usage' below for details.")
    parser.add_argument("game_engine", type=str, choices=[engine.value for engine in GameEngine], help="The game engine the plugin is for.")
    parser.add_argument("build_type", type=str, choices=[release_type.value for release_type in BuildType], help="The build type of the binaries to copy to the plugin.")
    parser.add_argument("--quiet", action="store_true", default=False, help="If true, will only output error messages. Info messages will not be written to stdout.")

    return parser.parse_args()


def configure_logging(quiet: bool):
    # Example of logging format:
    # INFO: Copying new AWS GameKit binaries from: D:\workspace\aws-gamekit\install\Debug\bin
    logging.basicConfig(
        format='%(levelname)s: %(message)s',
        level=logging.ERROR if quiet else logging.INFO
    )


def get_absolute_path_of_this_python_script() -> Path:
    return Path(os.path.dirname(os.path.abspath(__file__)))


def get_destination_binaries_dir(destination_plugin_root: Path, game_engine: GameEngine, build_type: BuildType) -> Path:
    if game_engine == GameEngine.UNREAL:
        return destination_plugin_root.joinpath('AwsGameKit', 'Libraries', 'Win64', build_type.value)
    elif game_engine == GameEngine.UNITY:
        # Unity doesn't support having separate Debug and Release binaries in the Plugins/ folder.
        return destination_plugin_root.joinpath('Assets', 'AWS GameKit', 'Plugins', 'Windows', 'x64')

    raise ValueError(f"Unsupported game_engine: {game_engine}")


def clean_destination(destination_binaries_dir: Path, destination_plugin_root: Path, game_engine: GameEngine):
    logging.info(f"Deleting AWS GameKit binaries from {game_engine.value}: {destination_binaries_dir}")
    delete_files(destination_binaries_dir, FilePatterns.DLL.value)
    delete_files(destination_binaries_dir, FilePatterns.PDB.value)
    if game_engine == GameEngine.UNREAL:
        unreal_binaries_dir = destination_plugin_root.joinpath('..', 'Binaries', 'Win64')
        logging.info(f"Deleting Unreal Binaries directory: {unreal_binaries_dir}")
        delete_directory_tree(unreal_binaries_dir)


def delete_files(directory: Path, file_pattern: str):
    if not directory.is_dir():
        return

    files_to_delete = [path for path in directory.iterdir() if fnmatch(path.name.casefold(), file_pattern)]
    for file in files_to_delete:
        file.unlink()


def delete_directory_tree(directory: Path):
    if directory.is_dir():
        shutil.rmtree(str(directory))


def copy_binaries_to_destination(source_binaries_dir: Path, destination_binaries_dir: Path, build_type: BuildType):
    logging.info(f"Copying new AWS GameKit binaries from: {source_binaries_dir}")
    copy_files(source_binaries_dir, destination_binaries_dir, FilePatterns.DLL.value)
    if build_type == BuildType.DEBUG:
        copy_files(source_binaries_dir, destination_binaries_dir, FilePatterns.PDB.value)


def copy_files(source_directory: Path, destination_directory: Path, file_pattern: str):
    if not destination_directory.is_dir():
        destination_directory.mkdir(parents=True)

    files_to_copy = [path for path in source_directory.iterdir() if fnmatch(path.name.casefold(), file_pattern)]
    files_to_copy.sort()
    for file in files_to_copy:
        logging.info(file)
        # Use copy2 to preserve metadata (such as creation timestamp and last modified timestamp).
        shutil.copy2(str(file), str(destination_directory))

    # Indentation makes it easier to read:
    indentation = '        '
    logging.info(f"{indentation}{len(files_to_copy)} file(s) copied.")


if __name__ == '__main__':
    main()
