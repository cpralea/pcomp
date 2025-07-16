import argparse
import sys

from dataclasses import dataclass
from math import ceil
from typing import Dict, List, Set, TextIO, Tuple

from asmspec import Instruction, AccessMode, Register


@dataclass
class VMInstrData:
    instr: Instruction
    am: AccessMode
    dst: Register | int | None  = None
    src: Register | int | None  = None
    idx: int | None             = None
    label: str | None           = None
    len: int                    = 0


LABEL_PREFIX: str = '.l'

hex_bytes_buffer: List[str] = []

cur_addr: int = 0
program: Dict[int, VMInstrData] = {}

cur_label_idx: int = 0
rev_label_addr: Dict[int, str] = {}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='VM disassembler.')
    parser.add_argument('input_file', metavar='HEX', type=str, nargs='?', \
                        help='input file to process; defaults to STDIN if unspecified')
    parser.add_argument('-o', '--output', metavar='ASM', type=str, dest='output_file', \
                        required=False, \
                        help='output file to emit VM asm to; defaults to STDOUT if unspecified')
    parser.add_argument('-l', '--labels', metavar='LBL', type=str, dest='labels_file', \
                        required=False, \
                        help='input file to read assembler labels from; defaults to none if unspecified')
    return parser.parse_args()


def unpack_opcode(hex_byte: str) -> Tuple[Instruction, AccessMode]:
    instr: Instruction = Instruction((int(hex_byte, base=16) & ~0x03) >> 2)
    am: AccessMode = AccessMode(int(hex_byte, base=16) & 0x03)
    return instr, am


def load_hex_bytes(input: TextIO):
    if hex_bytes_buffer != []:
        return
    line: str = input.readline().strip()
    if line == '':
        return
    hex_bytes_buffer.extend(line.split(' '))


def peek_hex_byte(input: TextIO) -> str:
    if hex_bytes_buffer == []:
        load_hex_bytes(input)
    if hex_bytes_buffer == []:
        return ''
    return hex_bytes_buffer[0]


def peek_opcode(input: TextIO) -> Tuple[Instruction, AccessMode] | None:
    hex_byte: str = peek_hex_byte(input)
    if not hex_byte:
        return None
    return unpack_opcode(hex_byte)


def peek_instr(input: TextIO) -> Instruction | None:
    opcode: Tuple[Instruction, AccessMode] | None = peek_opcode(input)
    if opcode is None:
        return None
    instr, _ = opcode
    return instr


def get_hex_byte(input: TextIO) -> str:
    hex_byte: str = peek_hex_byte(input)
    if hex_byte:
        hex_bytes_buffer.pop(0)
    return hex_byte


def disasm_opcode(input: TextIO) -> Tuple[Instruction, AccessMode]:
    return unpack_opcode(get_hex_byte(input))


def disasm_reg_reg(input: TextIO) -> Tuple[Register, Register]:
    rr: int = int(get_hex_byte(input), base=16)
    return Register(rr >> 4), Register(rr & 0x0f)


def disasm_imm(input: TextIO, width: int, signed: bool) -> int:
    hex_bytes: List[str] = []
    for _ in range(ceil(width/8)):
        hex_bytes.append(get_hex_byte(input))
    return int.from_bytes(bytes.fromhex(''.join(hex_bytes)), byteorder='little', signed=signed)


def disasm_instr(input: TextIO) -> VMInstrData:
    instr, am = disasm_opcode(input)
    return VMInstrData(instr, am, len=1)


def disasm_instr_r(input: TextIO) -> VMInstrData:
    instr, am = disasm_opcode(input)
    dst, _ = disasm_reg_reg(input)
    return VMInstrData(instr, am, dst=dst, len=2)


def disasm_instr_i(input: TextIO, signed: bool) -> VMInstrData:
    instr, am = disasm_opcode(input)
    return VMInstrData(instr, am, dst=disasm_imm(input, 64, signed=signed), len=9)


def disasm_instr_rr(input: TextIO) -> VMInstrData:
    instr, am = disasm_opcode(input)
    dst, src = disasm_reg_reg(input)
    return VMInstrData(instr, am, dst=dst, src=src, len=2)


def disasm_instr_ri(input: TextIO, signed: bool) -> VMInstrData:
    instr, am = disasm_opcode(input)
    dst, _ = disasm_reg_reg(input)
    return VMInstrData(instr, am, dst=dst, src=disasm_imm(input, 64, signed=signed), len=10)


def disasm_instr_rr_idx(input: TextIO, signed: bool) -> VMInstrData:
    instr, am = disasm_opcode(input)
    dst, src = disasm_reg_reg(input)
    idx = disasm_imm(input, 16, signed=signed)
    return VMInstrData(instr, am, dst=dst, src=src, idx=idx, len=4)


def disasm_instr_src_dst_idx(input: TextIO, signed: bool) -> VMInstrData:
    opcode: Tuple[Instruction, AccessMode] | None = peek_opcode(input)
    assert opcode is not None
    _, am = opcode
    match am:
        case AccessMode.REG:
            return disasm_instr_rr(input)
        case AccessMode.IMM:
            return disasm_instr_ri(input, signed=signed)
        case AccessMode.REG_IDX:
            return disasm_instr_rr_idx(input, signed=signed)


def disasm_load(input: TextIO) -> VMInstrData:
    return disasm_instr_src_dst_idx(input, signed=True)
def disasm_store(input: TextIO) -> VMInstrData:
    return disasm_instr_src_dst_idx(input, signed=True)
def disasm_mov(input: TextIO) -> VMInstrData:
    return disasm_instr_src_dst_idx(input, signed=True)
def disasm_add(input: TextIO) -> VMInstrData:
    return disasm_instr_src_dst_idx(input, signed=True)
def disasm_sub(input: TextIO) -> VMInstrData:
    return disasm_instr_src_dst_idx(input, signed=True)
def disasm_and(input: TextIO) -> VMInstrData:
    return disasm_instr_src_dst_idx(input, signed=False)
def disasm_or(input: TextIO) -> VMInstrData:
    return disasm_instr_src_dst_idx(input, signed=False)
def disasm_xor(input: TextIO) -> VMInstrData:
    return disasm_instr_src_dst_idx(input, signed=False)
def disasm_not(input: TextIO) -> VMInstrData:
    return disasm_instr_r(input)
def disasm_cmp(input: TextIO) -> VMInstrData:
    return disasm_instr_src_dst_idx(input, signed=True)
def disasm_push(input: TextIO) -> VMInstrData:
    return disasm_instr_r(input)
def disasm_pop(input: TextIO) -> VMInstrData:
    return disasm_instr_r(input)
def disasm_call(input: TextIO) -> VMInstrData:
    return disasm_instr_i(input, signed=False)
def disasm_ret(input: TextIO) -> VMInstrData:
    return disasm_instr(input)
def disasm_jmp(input: TextIO) -> VMInstrData:
    return disasm_instr_i(input, signed=False)
def disasm_jmpz(input: TextIO) -> VMInstrData:
    return disasm_instr_i(input, signed=False)
def disasm_jmpnz(input: TextIO) -> VMInstrData:
    return disasm_instr_i(input, signed=False)
def disasm_jmpeq(input: TextIO) -> VMInstrData:
    return disasm_instr_i(input, signed=False)
def disasm_jmpne(input: TextIO) -> VMInstrData:
    return disasm_instr_i(input, signed=False)
def disasm_jmpgt(input: TextIO) -> VMInstrData:
    return disasm_instr_i(input, signed=False)
def disasm_jmplt(input: TextIO) -> VMInstrData:
    return disasm_instr_i(input, signed=False)
def disasm_jmpge(input: TextIO) -> VMInstrData:
    return disasm_instr_i(input, signed=False)
def disasm_jmple(input: TextIO) -> VMInstrData:
    return disasm_instr_i(input, signed=False)


def disasm_instruction(input: TextIO) -> VMInstrData | None:
    instr: Instruction | None = peek_instr(input)
    if instr is None:
        return None
    match instr:
        case Instruction.LOAD:
            return disasm_load(input)
        case Instruction.STORE:
            return disasm_store(input)
        case Instruction.MOV:
            return disasm_mov(input)
        case Instruction.ADD:
            return disasm_add(input)
        case Instruction.SUB:
            return disasm_sub(input)
        case Instruction.AND:
            return disasm_and(input)
        case Instruction.OR:
            return disasm_or(input)
        case Instruction.XOR:
            return disasm_xor(input)
        case Instruction.NOT:
            return disasm_not(input)
        case Instruction.CMP:
            return disasm_cmp(input)
        case Instruction.PUSH:
            return disasm_push(input)
        case Instruction.POP:
            return disasm_pop(input)
        case Instruction.CALL:
            return disasm_call(input)
        case Instruction.RET:
            return disasm_ret(input)
        case Instruction.JMP:
            return disasm_jmp(input)
        case Instruction.JMPEQ:
            return disasm_jmpeq(input)
        case Instruction.JMPNE:
            return disasm_jmpne(input)
        case Instruction.JMPGT:
            return disasm_jmpgt(input)
        case Instruction.JMPLT:
            return disasm_jmplt(input)
        case Instruction.JMPGE:
            return disasm_jmpge(input)
        case Instruction.JMPLE:
            return disasm_jmple(input)
        case _: # pyright: reportUnnecessaryComparison=false
            sys.exit(f"Instruction '{instr}' not supported yet.")


def disasm_file(input: TextIO):
    global cur_addr
    instr_data: VMInstrData | None = disasm_instruction(input)
    while instr_data is not None:
        program[cur_addr] = instr_data
        cur_addr += instr_data.len
        instr_data = disasm_instruction(input)


def load_label_data(labels: TextIO):
    for label in labels:
        addr, label = label.strip().split('   ')
        addr = int(addr, base=16)
        rev_label_addr[addr] = label


def compute_label_data():
    addrs: Set[int] = set()
    for addr, data in program.items():
        if data.am == AccessMode.IMM and type(data.dst) == int and data.dst in program.keys():
            addrs.add(data.dst) # type: ignore
    for addr in sorted(addrs):
        global cur_label_idx
        cur_label_idx += 1
        rev_label_addr[addr] = f"{LABEL_PREFIX}{cur_label_idx}"


def apply_label_data():
    for addr, data in program.items():
        if addr in rev_label_addr.keys():
            data.label = rev_label_addr[addr]


def dump_program(output: TextIO):
    for _, data in program.items():

        instr: Instruction                  = data.instr
        dst: Register | int | str | None    = None
        src: Register | int | None          = None
        idx: int | None                     = None

        if type(data.dst) == int:
            addr: int = data.dst                                            # type: ignore
            if addr in program.keys():
                dst = program[addr].label
            elif addr in rev_label_addr.keys():
                dst = rev_label_addr[addr]
        elif type(data.dst) == Register:
            dst = data.dst
        if type(data.src) == int or type(data.src) == Register:
            src = data.src
        idx = data.idx

        asm: str = ''
        if data.label is not None:
            asm += f"{data.label}:\n"
        asm += f"    {instr}"
        if dst is not None:
            if instr == Instruction.STORE:
                assert idx is not None
                asm += f" [{dst} {'+' if idx >= 0 else '-'} {abs(idx)}]" if idx else f" [{dst}"
            else:
                asm += f" {dst}"
        if src is not None:
            if instr == Instruction.LOAD:
                assert idx is not None
                asm += f", [{src} {'+' if idx >= 0 else '-'} {abs(idx)}]" if idx else f", [{src}]"
            else:
                asm += f", {src}"

        print(asm.lower(), file=output)



def disassemble():
    args = parse_args()

    input_file: TextIO = sys.stdin
    output_file: TextIO = sys.stdout
    labels_file: TextIO | None = None

    if args.input_file is not None:
        input_file = open(args.input_file, mode='r', encoding='utf-8')      # type: ignore
    if args.output_file is not None:
        output_file = open(args.output_file, mode='w', encoding='utf-8')    # type: ignore
    if args.labels_file is not None:
        labels_file = open(args.labels_file, mode='r', encoding='utf-8')    # type: ignore

    with output_file as output:
        with input_file as input:
            disasm_file(input)
            if labels_file is not None:
                with labels_file as labels:
                    load_label_data(labels)
            else:
                compute_label_data()
            apply_label_data()
            dump_program(output)


disassemble()
