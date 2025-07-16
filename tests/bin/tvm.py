import argparse

from typing import List

from utils import *


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Test the VM.')
    parser.add_argument('-r', '--root', metavar='ROOT', type=str, dest='root_dir', \
                        required=False, default='tests/data/vm', \
                        help='root directory for in/*.asm and ref/*.stdout')
    parser.add_argument('-e', '--execution-type', metavar='EXEC_TYPE', dest='exec_type',
                        required=False, choices=['INTERPRETER', 'AArch64JIT', 'x86_64JIT'], default='INTERPRETER',
                        help='''the execution type; defaults to INTERPRETER;
                                possible values: INTERPRETER, AArch64JIT, x86_64JIT''')
    return parser.parse_args()


def execute_test(name: str, exec_type: str, in_asm: str, ref_stdout: str, out_hex: str, out_stdout: str):
    print(f"{name}...", end='')

    if not execute(f"python3 $PCOMP_DEVROOT/tools/asm.py -o {out_hex} {in_asm}"):
        print_red('failed')
        return
    if not execute(f"source env.sh && python3 $PCOMP_DEVROOT/tools/vm.py -e {exec_type} {out_hex} > {out_stdout}"):
        print_red('failed')
        return
    if not execute(f"diff {ref_stdout} {out_stdout}"):
        print_red('failed')
        return

    print_green('pass')


def execute_tests():
    args: argparse.Namespace = parse_args()

    exec_type: str                          = args.exec_type

    in_dir: str                             = f"{args.root_dir}/in"
    ref_dir: str                            = f"{args.root_dir}/ref"
    out_dir: str                            = create_tmpdir('asm2stdout-')

    names: List[str]                        = [f"{file.rpartition('.')[0]}" for file in list_files(in_dir, '.asm')]
    in_asm_files: List[str]                 = [f"{in_dir}/{name}.asm" for name in names]
    ref_stdout_files: List[str]             = [f"{ref_dir}/{name}.stdout" for name in names]
    out_hex_files: List[str]                = [f"{out_dir}/{name}.hex" for name in names]
    out_stdout_files: List[str]             = [f"{out_dir}/{name}.stdout" for name in names]

    tests: zip[tuple[str, str, str, str, str]] \
        = zip(names, in_asm_files, ref_stdout_files, out_hex_files, out_stdout_files)

    print_green(f"*.asm -> *.stdout ({exec_type.lower()})")
    for name, in_asm, ref_stdout, out_hex, out_stdout in tests:
        execute_test(name, exec_type, in_asm, ref_stdout, out_hex, out_stdout)
    
    remove_dir(out_dir)


execute_tests()
