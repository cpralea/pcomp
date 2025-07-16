import argparse

from typing import List

from utils import *


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='Test VM assembler/disassembler roundtrip.')
    parser.add_argument('-r', '--root', metavar='ROOT', type=str, dest='root_dir', \
                        required=False, default='tests/data/vm', \
                        help='root directory for in/*.asm')
    return parser.parse_args()


def execute_test(name: str, in_asm: str, ref_sys_enter: str, ref_asm: str, out_hex: str, out_lbl: str, out_asm: str):
    print(f"{name}...", end='')

    if not execute(f"cp {ref_sys_enter} {ref_asm}"):
        print_red('failed')
        return
    if not execute(f"python3 $PCOMP_DEVROOT/tests/bin/fstrip.py < {in_asm} >> {ref_asm}"):
        print_red('failed')
        return
    if not execute(f"python3 $PCOMP_DEVROOT/tools/asm.py -o {out_hex} -l {out_lbl} {in_asm}"):
        print_red('failed')
        return
    if not execute(f"python3 $PCOMP_DEVROOT/tools/disasm.py -o {out_asm} -l {out_lbl} {out_hex}"):
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
    out_dir: str                            = create_tmpdir('asm2asm-')

    names: List[str]                        = [f"{file.rpartition('.')[0]}" for file in list_files(in_dir, '.asm')]
    in_asm_files: List[str]                 = [f"{in_dir}/{name}.asm" for name in names]
    out_hex_files: List[str]                = [f"{out_dir}/{name}.hex" for name in names]
    out_lbl_files: List[str]                = [f"{out_dir}/{name}.lbl" for name in names]
    out_asm_files: List[str]                = [f"{out_dir}/{name}.asm" for name in names]

    ref_sys_enter: str                      = f"{ref_dir}/sys_enter.asm"
    ref_asm: str                            = f"{out_dir}/ref.asm"

    tests: zip[tuple[str, str, str, str, str]] \
        = zip(names, in_asm_files, out_hex_files, out_lbl_files, out_asm_files)

    print_green("*.asm -> *.{hex,lbl} -> *.asm")
    for name, in_asm, out_hex, out_lbl, out_asm in tests:
        execute_test(name, in_asm, ref_sys_enter, ref_asm, out_hex, out_lbl, out_asm)
    
    remove_dir(out_dir)


execute_tests()
