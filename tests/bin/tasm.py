import argparse

from typing import List

from utils import *


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Test the VM assembler.')
    parser.add_argument('-r', '--root', metavar='ROOT', type=str, dest='root_dir', \
                        required=False, default='tests/data/asm', \
                        help='root directory for in/*.asm and ref/*.{hex,lbl}')
    return parser.parse_args()


def execute_test(name: str, in_asm: str, ref_hex: str, ref_lbl: str, out_hex: str, out_lbl: str):
    print(f"{name}...", end='')

    if not execute(f"python3 $PCOMP_DEVROOT/tools/asm.py -o {out_hex} -l {out_lbl} {in_asm}"):
        print_red('failed')
        return
    if not (execute(f"diff {ref_hex} {out_hex}") and execute(f"diff {ref_lbl} {out_lbl}")):
        print_red('failed')
        return
    
    print_green('pass')


def execute_tests():
    args: argparse.Namespace = parse_args()

    in_dir: str                             = f"{args.root_dir}/in"
    ref_dir: str                            = f"{args.root_dir}/ref"
    out_dir: str                            = create_tmpdir('asm2hex-')

    names: List[str]                        = [f"{file.rpartition('.')[0]}" for file in list_files(in_dir, '.asm')]
    in_asm_files: List[str]                 = [f"{in_dir}/{name}.asm" for name in names]
    ref_hex_files: List[str]                = [f"{ref_dir}/{name}.hex" for name in names]
    ref_lbl_files: List[str]                = [f"{ref_dir}/{name}.lbl" for name in names]
    out_hex_files: List[str]                = [f"{out_dir}/{name}.hex" for name in names]
    out_lbl_files: List[str]                = [f"{out_dir}/{name}.lbl" for name in names]

    tests: zip[tuple[str, str, str, str, str, str]] \
        = zip(names, in_asm_files, ref_hex_files, ref_lbl_files, out_hex_files, out_lbl_files)

    print_green("*.asm -> *.{hex,lbl}")
    for name, in_asm, ref_hex, ref_lbl, out_hex, out_lbl in tests:
        execute_test(name, in_asm, ref_hex, ref_lbl, out_hex, out_lbl)
    
    remove_dir(out_dir)


execute_tests()
