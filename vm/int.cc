#include <cstdlib>
#include <cstring>

#include "exe.h"
#include "int.h"


#define DISPATCH(OFFSET) { \
    reg[PC] += OFFSET; \
    goto *instr_exec_handle[instr(mem[reg[PC]])]; \
}

#define TRACE() if (debug) { idd.addr = reg[PC]; trace_instr_decode(mem, idd); }


Interpreter::Interpreter(const void* prog, size_t prog_size, size_t mem_size_mb, bool debug)
: ExecutionEngine(prog, prog_size, mem_size_mb, debug)
, mem(nullptr)
{
    DBG("\ttype 'interpreter'" << endl);
}


void Interpreter::init_execution()
{
    DBG("Initializing memory ..." << endl);
    mem = new uint8_t[mem_size];
    DBG("\tMemory @" << (void*) mem << "[" << HEX(0, mem_size) << "]" << endl);

    DBG("Initializing registers ..." << endl);
    std::memset(&reg, 0, sizeof reg);
    reg[SP] = mem_size;
    reg[PC] = 9;
}


void Interpreter::load_program()
{
    DBG("Loading program ..." << endl);
    std::memmove(mem, prog, prog_size);
}


void Interpreter::exec_program()
{
    if (prog_size <= reg[PC])
        return;

    DBG("Running program ..." << endl);

    static void* instr_exec_handle[] = {
        nullptr,
        &&_load,
        &&_store,
        &&_mov,
        &&_add,
        &&_sub,
        &&_and,
        &&_or,
        &&_xor,
        &&_not,
        &&_cmp,
        &&_push,
        &&_pop,
        &&_call,
        &&_ret,
        &&_jmp,
        &&_jmpeq,
        &&_jmpne,
        &&_jmpgt,
        &&_jmplt,
        &&_jmpge,
        &&_jmple,
    };

    instr_decode_data_t idd = { 0, 0, 0, 0, 0, 0, 0 };

    DISPATCH(+0);

    _load: {
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        idd.src = reg_src(mem[reg[PC] + 1]);
        idd.idx = imm16s(mem[reg[PC] + 2]);
        TRACE();
        reg[idd.dst] = as_dword(mem[reg[idd.src] + idd.idx]);
        DISPATCH(+4);
    }

    _store: {
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        idd.src = reg_src(mem[reg[PC] + 1]);
        idd.idx = imm16s(mem[reg[PC] + 2]);
        TRACE();
        as_dword(mem[reg[idd.dst] + idd.idx]) = reg[idd.src];
        DISPATCH(+4);
    }

    _mov: {
        idd.am = access_mode(mem[reg[PC]]);
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        switch (idd.am) {
        case REG:
            idd.src = reg_src(mem[reg[PC] + 1]);
            TRACE();
            reg[idd.dst] = reg[idd.src];
            DISPATCH(+2);
        case IMM:
            idd.ivs = imm64s(mem[reg[PC] + 2]);
            TRACE();
            as_signed(reg[idd.dst]) = idd.ivs;
            DISPATCH(+10);
        }
    }

    _add: {
        idd.am = access_mode(mem[reg[PC]]);
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        switch (idd.am) {
        case REG:
            idd.src = reg_src(mem[reg[PC] + 1]);
            TRACE();
            as_signed(reg[idd.dst]) += as_signed(reg[idd.src]);
            DISPATCH(+2);
        case IMM:
            idd.ivs = imm64s(mem[reg[PC] + 2]);
            TRACE();
            as_signed(reg[idd.dst]) += idd.ivs;
            DISPATCH(+10);
        }
    }

    _sub: {
        idd.am = access_mode(mem[reg[PC]]);
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        switch (idd.am) {
        case REG:
            idd.src = reg_src(mem[reg[PC] + 1]);
            TRACE();
            as_signed(reg[idd.dst]) -= as_signed(reg[idd.src]);
            DISPATCH(+2);
        case IMM:
            idd.ivs = imm64s(mem[reg[PC] + 2]);
            TRACE();
            as_signed(reg[idd.dst]) -= idd.ivs;
            DISPATCH(+10);
        }
    }

    _and: {
        idd.am = access_mode(mem[reg[PC]]);
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        switch (idd.am) {
        case REG:
            idd.src = reg_src(mem[reg[PC] + 1]);
            TRACE();
            reg[idd.dst] &= reg[idd.src];
            DISPATCH(+2);
        case IMM:
            idd.ivu = imm64u(mem[reg[PC] + 2]);
            TRACE();
            reg[idd.dst] &= idd.ivu;
            DISPATCH(+10);
        }
    }

    _or: {
        idd.am = access_mode(mem[reg[PC]]);
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        switch (idd.am) {
        case REG:
            idd.src = reg_src(mem[reg[PC] + 1]);
            TRACE();
            reg[idd.dst] |= reg[idd.src];
            DISPATCH(+2);
        case IMM:
            idd.ivu = imm64u(mem[reg[PC] + 2]);
            TRACE();
            reg[idd.dst] |= idd.ivu;
            DISPATCH(+10);
        }
    }

    _xor: {
        idd.am = access_mode(mem[reg[PC]]);
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        switch (idd.am) {
        case REG:
            idd.src = reg_src(mem[reg[PC] + 1]);
            TRACE();
            reg[idd.dst] ^= reg[idd.src];
            DISPATCH(+2);
        case IMM:
            idd.ivu = imm64u(mem[reg[PC] + 2]);
            TRACE();
            reg[idd.dst] ^= idd.ivu;
            DISPATCH(+10);
        }
    }

    _not: {
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        TRACE();
        reg[idd.dst] = ~reg[idd.dst];
        DISPATCH(+2);
    }

    _cmp: {
        idd.am = access_mode(mem[reg[PC]]);
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        switch (idd.am) {
        case REG:
            idd.src = reg_src(mem[reg[PC] + 1]);
            TRACE();
            reg[FLAGS] = 0;
            if (as_signed(reg[idd.dst]) < as_signed(reg[idd.src]))
                reg[FLAGS] |= FLAG_LT;
            else if (as_signed(reg[idd.dst]) > as_signed(reg[idd.src]))
                reg[FLAGS] |= FLAG_GT;
            else
                reg[FLAGS] |= FLAG_EQ;
            DISPATCH(+2);
        case IMM:
            idd.ivs = imm64s(mem[reg[PC] + 2]);
            TRACE();
            reg[FLAGS] = 0;
            if (as_signed(reg[idd.dst]) < idd.ivs)
                reg[FLAGS] |= FLAG_LT;
            else if (as_signed(reg[idd.dst]) > idd.ivs)
                reg[FLAGS] |= FLAG_GT;
            else
                reg[FLAGS] |= FLAG_EQ;
            DISPATCH(+10);
        }
    }

    _push: {
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        TRACE();
        reg[SP] -= 8;
        as_dword(mem[reg[SP]]) = reg[idd.dst];
        DISPATCH(+2);
    }

    _pop: {
        idd.dst = reg_dst(mem[reg[PC] + 1]);
        TRACE();
        reg[idd.dst] = as_dword(mem[reg[SP]]);
        reg[SP] += 8;
        DISPATCH(+2);
    }

    _call: {
        idd.ivu = imm64u(mem[reg[PC] + 1]);
        TRACE();
        reg[SP] -= 8;
        as_dword(mem[reg[SP]]) = reg[PC] + 9;
        reg[PC] = idd.ivu;
        DISPATCH(+0);
    }

    _ret: {
        TRACE();
        reg[PC] = as_dword(mem[reg[SP]]);
        reg[SP] += 8;
        DISPATCH(+0);
    }

    _jmp: {
        idd.ivu = imm64u(mem[reg[PC] + 1]);
        TRACE();
        switch (reg[PC]) {
        case SYS_ENTER_ADDR: {
            uint64_t syscall_id = imm64u(mem[reg[SP] + 8]);
            switch (syscall_id) {
            case SYSCALL_VM_EXIT:
                reg[SP] += 16;
                return;
            default:
                sys_enter();
                goto _ret;
            }
        }
        default:
            reg[PC] = idd.ivu;
            DISPATCH(+0);
        }
    }

    _jmpeq: {
        idd.ivu = imm64u(mem[reg[PC] + 1]);
        TRACE();
        if (reg[FLAGS] & FLAG_EQ) {
            reg[PC] = idd.ivu;
            DISPATCH(+0);
        } else {
            DISPATCH(+9);
        }
    }

    _jmpne: {
        idd.ivu = imm64u(mem[reg[PC] + 1]);
        TRACE();
        if (reg[FLAGS] & FLAG_EQ) {
            DISPATCH(+9);
        } else {
            reg[PC] = idd.ivu;
            DISPATCH(+0);
        }
    }

    _jmpgt: {
        idd.ivu = imm64u(mem[reg[PC] + 1]);
        TRACE();
        if (reg[FLAGS] & FLAG_GT) {
            reg[PC] = idd.ivu;
            DISPATCH(+0);
        } else {
            DISPATCH(+9);
        }
    }

    _jmplt: {
        idd.ivu = imm64u(mem[reg[PC] + 1]);
        TRACE();
        if (reg[FLAGS] & FLAG_LT) {
            reg[PC] = idd.ivu;
            DISPATCH(+0);
        } else {
            DISPATCH(+9);
        }
    }

    _jmpge: {
        idd.ivu = imm64u(mem[reg[PC] + 1]);
        TRACE();
        if (reg[FLAGS] & (FLAG_GT | FLAG_EQ)) {
            reg[PC] = idd.ivu;
            DISPATCH(+0);
        } else {
            DISPATCH(+9);
        }
    }

    _jmple: {
        idd.ivu = imm64u(mem[reg[PC] + 1]);
        TRACE();
        if (reg[FLAGS] & (FLAG_LT | FLAG_EQ)) {
            reg[PC] = idd.ivu;
            DISPATCH(+0);
        } else {
            DISPATCH(+9);
        }
    }

    ABORT("Runaway interpreter execution." << endl);
}


void Interpreter::fini_execution()
{
    delete[] mem;
    dump_registers();
}


void Interpreter::sys_enter()
{
    uint64_t syscall_id = imm64u(mem[reg[SP] + 8]);
    switch (syscall_id) {
    case SYSCALL_VM_EXIT:
        ABORT("Internal error. SYSCALL_VM_EXIT should not have been handled here." << endl);
    case SYSCALL_DISPLAY_SINT: {
        int64_t val = imm64s(mem[reg[SP] + 16]);
        cout << val << endl;
        as_dword(mem[reg[SP] + 16]) = as_dword(mem[reg[SP]]);
        reg[SP] += 16;
        break;
    }
    case SYSCALL_DISPLAY_UINT: {
        uint64_t val = imm64u(mem[reg[SP] + 16]);
        cout << val << endl;
        as_dword(mem[reg[SP] + 16]) = as_dword(mem[reg[SP]]);
        reg[SP] += 16;
        break;
    }
    default:
        ABORT("Unsupported syscall ID '" << syscall_id << "'." << endl);
    }
}


void Interpreter::dump_registers() const
{
    DBG("Registers:" << endl);
    DBG("\tR0    = " << HEX(16, reg[R0])    << endl);
    DBG("\tR1    = " << HEX(16, reg[R1])    << endl);
    DBG("\tR2    = " << HEX(16, reg[R2])    << endl);
    DBG("\tR3    = " << HEX(16, reg[R3])    << endl);
    DBG("\tR4    = " << HEX(16, reg[R4])    << endl);
    DBG("\tR5    = " << HEX(16, reg[R5])    << endl);
    DBG("\tR6    = " << HEX(16, reg[R6])    << endl);
    DBG("\tR7    = " << HEX(16, reg[R7])    << endl);
    DBG("\tR8    = " << HEX(16, reg[R8])    << endl);
    DBG("\tR9    = " << HEX(16, reg[R9])    << endl);
    DBG("\tR10   = " << HEX(16, reg[R10])   << endl);
    DBG("\tR11   = " << HEX(16, reg[R11])   << endl);
    DBG("\tR12   = " << HEX(16, reg[R12])   << endl);
    DBG("\tFLAGS = " << HEX(16, reg[FLAGS]) << endl);
    DBG("\tSP    = " << HEX(16, reg[SP])    << endl);
    DBG("\tPC    = " << HEX(16, reg[PC])    << endl);
}
