# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

from enum import Enum
import json
import logging
import pathlib
import shutil
import subprocess
import os

FEATURES = ["core", "achievements", "identity", "game-saving", "user-gameplay-data", "authentication"]

REPOSITORY_ROOT = pathlib.Path(__file__).absolute().parents[1]
GAMEKIT_ENV_FILE = pathlib.Path(REPOSITORY_ROOT / ".env")

def run_and_log(command: list):
    logging.basicConfig(level=logging.INFO)
    logging.info(f"Running command: {command} from directory: {os.getcwd()}")
    try:
        output = subprocess.check_output(command)
        for line in output.decode('utf-8').split("\n"):
            logging.info(line)
    except FileNotFoundError as err:
        logging.error(f"Command: {command} from directory: {os.getcwd()} failed. Could not find command on path.")
        raise err
    except subprocess.CalledProcessError as err:
        logging.error(f"Command: {command} from directory: {os.getcwd()} failed.")
        for line in err.output.decode('utf-8').split('\n'):
            logging.info(line)
        raise err

def __in_gamekit_env__(key):
    if GAMEKIT_ENV_FILE.exists():
        with open(GAMEKIT_ENV_FILE, "r") as e:
            contents = e.read()
            if contents.strip() == "":
                contents = "{}"
            env_contents = json.loads(contents)
        return key in env_contents.keys()
    return False

def __get_gamekit_env_value__(key):
    with open(GAMEKIT_ENV_FILE, "r") as e:
        contents = e.read()
        if contents.strip() == "":
            contents = "{}"
        env_contents = json.loads(contents)
        return env_contents[key]

def __set_gamekit_env__(key, val):
    GAMEKIT_ENV_FILE.touch()
    with open(GAMEKIT_ENV_FILE, "r") as e:
        contents = e.read()
        if contents.strip() == "":
            contents = "{}"
        env_contents = json.loads(contents)
        env_contents[key] = str(val)
    with open(GAMEKIT_ENV_FILE, "w") as e:
        e.write(json.dumps(env_contents, indent=4))

def set_env_path(key: str, prompt: str):
    if key in os.environ:
        return os.getenv(key)
    else:
        if __in_gamekit_env__(key):
            return __get_gamekit_env_value__(key)
        
        # else get a new path, write it to the .env file, and then return it
        val = input(prompt)
        val = pathlib.Path(val.strip()).expanduser()
        if val.exists():
            __set_gamekit_env__(key, val)
            return str(val)
        else:
            logging.error(f"Could not set env variable {key} .. Path does not exist: {val}")

def code_sign(binary, certificate_name):
    # Sign lib with a "Developer Id Application certificate" so it can be distributed outside the app store after being notarized.
    subprocess.run(f'codesign --timestamp --options=runtime --force -s "{certificate_name}" {binary}', shell=True)

def copy_file(source: pathlib.Path, destination: pathlib.Path):
    if not destination.is_dir():
        destination.mkdir(parents=True)
    new_file = destination / source.name
    if new_file.exists():
        # Delete existing file with same name if it exists
        logging.info(f"Deleting existing file {new_file}")
        new_file.unlink()

    logging.info(f"Copying {source} to {destination}")
    # Use copy2 to preserve metadata (such as creation timestamp and last modified timestamp).
    shutil.copy2(str(source), str(destination))

def copy_libs(file_type: str, destination: pathlib.Path, binary_dir=REPOSITORY_ROOT):
    files_to_copy = []
    for binary in binary_dir.iterdir():
        files_to_copy += [binary] if pathlib.Path(binary).name.endswith(file_type) else []
    files_to_copy.sort()
    for file in files_to_copy:
        copy_file(file, destination)

    logging.info(f"\t{len(files_to_copy)} file(s) copied.")
