import os
import shutil
import subprocess
import tempfile

from typing import List


def list_files(dir_name: str, file_suffix: str) -> List[str]:
    return sorted([file for file in os.listdir(dir_name) if file.endswith(file_suffix)])


def create_tmpdir(dir_prefix: str) -> str:
    return tempfile.mkdtemp(prefix=dir_prefix)


def remove_dir(dir_name: str):
    shutil.rmtree(dir_name, ignore_errors=True)


def print_red(msg: str, end: str | None = None):
    print(f"\033[91m{msg}\033[0m", end=end)
def print_green(msg: str, end: str | None = None):
    print(f"\033[92m{msg}\033[0m", end=end)


def execute(cmd: str) -> bool:
    process = subprocess.run(cmd, shell=True, executable='/bin/bash')
    return True if process.returncode == 0 else False
