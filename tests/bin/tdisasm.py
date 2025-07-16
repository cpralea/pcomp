import argparse

from typing import List

from utils import *


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Test the VM disassembler.')
    parser.add_argument('-r', '--root', metavar='ROOT', type=str, dest='root_dir', \
                        required=False, default='tests/data/disasm', \
                        help='root directory for in/*.{hex,lbl} and ref/*.asm')
    return parser.parse_args()


def execute_test(name: str, in_hex: str, in_lbl: str, ref_asm: str, out_asm: str):
    print(f"{name}...", end='')

    if not execute(f"python3 $PCOMP_DEVROOT/tools/disasm.py -o {out_asm} -l {in_lbl} {in_hex}"):
        print_red('failed')
        return
    if not execute(f"diff {ref_asm} {out_asm}"):
        print_red('failed')
        return
    
    print_green('pass')


def execute_tests():
    args: argparse.Namespace = parse_args()

    in_dir: str                             = f"{args.root_dir}/in"
    ref_dir: str                            = f"{args.root_dir}/ref"
    out_dir: str                            = create_tmpdir('hex2asm-')

    names: List[str]                        = [f"{file.rpartition('.')[0]}" for file in list_files(in_dir, '.hex')]
    in_hex_files: List[str]                 = [f"{in_dir}/{name}.hex" for name in names]
    in_lbl_files: List[str]                 = [f"{in_dir}/{name}.lbl" for name in names]
    ref_asm_files: List[str]                = [f"{ref_dir}/{name}.asm" for name in names]
    out_asm_files: List[str]                = [f"{out_dir}/{name}.asm" for name in names]

    tests: zip[tuple[str, str, str, str, str]] \
        = zip(names, in_hex_files, in_lbl_files, ref_asm_files, out_asm_files)

    print_green("*.{hex,lbl} -> *.asm")
    for name, in_hex, in_lbl, ref_asm, out_asm in tests:
        execute_test(name, in_hex, in_lbl, ref_asm, out_asm)
    
    remove_dir(out_dir)


execute_tests()
