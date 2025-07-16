# Directory structure

## /doc
Documentation.
### /doc/vm.md
VM specification.

## /tools
Tools.
### /tools/asm.py
VM assembler.
```
(python) ucomp$ python3 tools/asm.py --help
usage: asm.py [-h] [-o HEX] [-l LBL] [ASM]

VM assembler.

positional arguments:
  ASM                   input file to process; defaults to STDIN if unspecified

options:
  -h, --help            show this help message and exit
  -o HEX, --output HEX  output file to emit VM code to; defaults to STDOUT if unspecified
  -l LBL, --labels LBL  output file to emit assembler labels to; defaults to none if unspecified
```
### /tools/disasm.py
VM disassembler.
```
(python) ucomp$ python3 tools/disasm.py --help
usage: disasm.py [-h] [-o ASM] [-l LBL] [HEX]

VM disassembler.

positional arguments:
  HEX                   input file to process; defaults to STDIN if unspecified

options:
  -h, --help            show this help message and exit
  -o ASM, --output ASM  output file to emit VM asm to; defaults to STDOUT if unspecified
  -l LBL, --labels LBL  input file to read assembler labels from; defaults to none if unspecified
```
### /tools/vm.py
VM wrapper.
```
(python) ucomp$ python3 tools/vm.py --help
usage: vm.py [-h] [-m MEM] [-e EXEC_TYPE] [-d] HEX

VM wrapper.

positional arguments:
  HEX                   the program to execute

options:
  -h, --help            show this help message and exit
  -m MEM, --memory MEM  the size of memory to use (in MiB); defaults to 4
  -e EXEC_TYPE, --execution-type EXEC_TYPE
                        the execution type; defaults to INTERPRETER; possible values: INTERPRETER,
                        AArch64JIT, x86_64JIT
  -d, --debug           emit debug info
```
## /tests
Tests.
### /tests/bin/tasm.py
Execute `*.asm` -> `*.{hex,lbl}` assembler tests.
### /tests/bin/tdisasm.py
Execute `*.{hex,lbl}` -> `*.asm` disassembler tests.
### /tests/bin/tasmroundtrip.py
Execute `*.asm` -> `*.{hex,lbl}` -> `*.asm` tests.
### /tests/bin/tvm.py
Execute VM tests.
### /tests/bin/trunall.py
Execute all tests.

## /vm
VM implementation.
## /vm/vm.{cc,h}
Entrypoint.
## /vm/exe.{cc,h}
Common execution functionality (interpreter / JIT).
## /vm/int.{cc,h}
Interpreter.
## /vm/jit.{cc,h}
Common JIT functionality (AArch64 / x86_64).
## /vm/a64.{cc,h}
AArch64 JIT.
## /vm/x64.{cc,h}
x86_64 JIT.

# Compilation
## Requirements
The VM was tested using the following software:
- GNU make 3.81
- Python 3.11.3
- clang 14.0.3
## Build
```
ucomp$ make venv
ucomp$ source env.sh     # customize it first to match the local setup
(python) ucomp$ make
```
## Test
```
(python) ucomp$ python3 tests/bin/trunall.py
```

# Execution
Compute 16!
```
(python) ucomp$ python3 tools/asm.py tests/data/vm/in/factorial.asm -o factorial.hex
(python) ucomp$ python3 tools/vm.py factorial.hex
20922789888000
```
