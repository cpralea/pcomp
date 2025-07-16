import argparse

from typing import List

from utils import *


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Test the CPU hardware implementation.')
    parser.add_argument('-r', '--root', metavar='ROOT', type=str, dest='root_dir', \
                        required=False, default='tests/data/hw', \
                        help='root directory for tb/*.v and ref/*.stdout')
    return parser.parse_args()


def execute_test(name: str, in_v: str, ref_stdout: str, out_dir: str, out_stdout: str):
    print(f"{name}...", end='')

    if not execute(f"python3 $PCOMP_DEVROOT/tools/hwsim.py {in_v} -o {out_dir}"):
        print_red('failed')
        return
    if not execute(f"diff {ref_stdout} {out_stdout}"):
        print_red('failed')
        return
    
    print_green('pass')


def execute_tests():
    args: argparse.Namespace = parse_args()

    in_dir: str                             = f"{args.root_dir}/tb"
    ref_dir: str                            = f"{args.root_dir}/ref"
    out_dir: str                            = create_tmpdir('v2vvp-')

    names: List[str]                        = [f"{file.rpartition('.')[0]}" for file in list_files(in_dir, '.v')]
    in_v_files: List[str]                   = [f"{in_dir}/{name}.v" for name in names]
    ref_stdout_files: List[str]             = [f"{ref_dir}/{name}.stdout" for name in names]
    out_stdout_files: List[str]             = [f"{out_dir}/{name}.stdout" for name in names]

    tests: zip[tuple[str, str, str, str]] \
        = zip(names, in_v_files, ref_stdout_files, out_stdout_files)

    print_green("*.v -> *.stdout")
    for name, in_v, ref_stdout, out_stdout in tests:
        execute_test(name, in_v, ref_stdout, out_dir, out_stdout)
    
    remove_dir(out_dir)


execute_tests()
