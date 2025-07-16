# Overview

64-bit RISC VM that supports a minimal set of instructions.


# Registers

- `R0`, `R1`, `R2`, ..., `R12`  
Thirteen, 64-bit general purpose registers.

- `SP`  
64-bit stack pointer.

- `FLAGS`  
Flag register. Set by execution of `CMP` instruction. Execution of all other instructions leave it in an undefined state.

- `PC`  
64-bit program counter.


# Instructions

- `LOAD <Rd>, [<Rs> (+|- <idx>)?]`  
Read the 64-bit value from memory at location `<Rs>` +/- an optional 16-bit `<idx>` into `<Rd>`.

- `STORE [<Rd> (+|- <idx>)?], <Rs>`  
Write the 64-bit value in `<Rs>` to the memory location at `<Rd>` +/- an optional 16-bit `<idx>`.

- `MOV <Rd>, <Rs>|<imm>`  
Copy `<Rs>` or `<imm>` to `<Rd>`.

- `ADD <Rd>, <Rs>|<imm>`  
Add `<Rs>` or `<imm>` to `<Rd>`. Store the result in `<Rd>`.

- `SUB <Rd>, <Rs>|<imm>`  
Subtract `<Rs>` or `<imm>` from `<Rd>`. Store the result in `<Rd>`.

- `AND <Rd>, <Rs>|<imm>`  
Bitwise `and` between `<Rd>` and `<Rs>` or `<imm>`. The result is stored in `<Rd>`.

- `OR <Rd>, <Rs>|<imm>`  
Bitwise `or` between `<Rd>` and `<Rs>` or `<imm>`. The result is stored in `<Rd>`.

- `XOR <Rd>, <Rs>|<imm>`
Bitwise `exclusive or` between `<Rd>` and `<Rs>` or `<imm>`. The result is stored in `<Rd>`.

- `NOT <Rd>`  
Invert in place all bits in `<Rd>`.

- `CMP <Rd>, <Rs>|<imm>`  
Arithmetic comparison of `<Rd>` with either `<Rs>` or `<imm>` setting `FLAGS`.

- `PUSH <Rs>`  
Subtract 8 from `SP` and store `<Rs>` at the memory location it points to.

- `POP <Rd>`  
Read a 64-bit value from the memory location `SP` points to, store it into `<Rd>` and add 8 to `SP`.

- `CALL <imm>`  
Perform a subroutine call to address `<imm>`. The address of the instruction following the current one is pushed onto the stack.

- `RET`  
Perform a return from a subroutine call by popping the address of the next instruction to be executed off of the stack and loading it into `PC`.

- `JMP<cond> <imm>`  
Continue execution with the instruction at address `<imm>` by loading `<imm>` into `PC`. If specified, check the `<cond>` against `FLAGS`. If the condition holds, perform the jump. If not, do not perform the jump and continue execution as normal.

where
- `<Rd>`/`<Rs>` are any 64-bit registers, except `FLAGS` and `PC`
- `<imm>` is any 64-bit immediate value
- `<idx>` is any 16-bit immediate value
- `[<reg> (+|- <idx>)?]` is the value at the memory location `<reg>` +/- `<idx>` points to; `<idx>` is optional and defaults to `0` if omitted
- `<cond>` either is missing or can be any of `EQ`, `NE`, `GT`, `LT`, `GE`, `LE`


# Instruction encoding

```
|  opcode  | 1st operand   2nd operand |
|__1 byte__|______1/3/8/9 bytes________|
```


# Opcode encoding

```
|  operation |     am     |
|___6 bits___|___2 bits___|
|_________1 byte__________|
```


# Operand encoding

```
|   reg    |        |    imm    |        |    idx    |
|__4 bits__|        |__8 bytes__|        |__2 bytes__|
```


# Calling convention

- all parameters passed on stack; cleared by the callee
- all return values passed on stack; cleared by the caller
- no registers are preserved between calls


# System calls

Regular `CALL` to one predefined address:  
```
CALL $sys_enter
```
Parameters are passed in on the stack, as usual, with the one on top representing the system call ID.

```
------------------
| nth parameter* |
|      ...       |
| 2nd parameter* |
| 1st parameter* |
| system call ID |
| return address | <--- top of the stack
|                |
```

- `SYSCALL_VM_EXIT` (ID == 0)  
Terminate execution.

- `SYSCALL_DISPLAY_SINT` (ID == 1)  
Display 1st parameter as an unsigned integer.

- `SYSCALL_DISPLAY_UINT` (ID == 2)  
Display 1st parameter as a signed integer.


# Memory layout

```
0x0000  JMP 0xXXXXXXXXXXXXXXXX      ; $sys_enter
0x0009                              ; text/data/heap/stack
```
