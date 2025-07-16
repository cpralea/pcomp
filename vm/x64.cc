#include <algorithm>
#include <map>

#include "exe.h"
#include "x64.h"


#define JIT_NEXT(OFFSET) { \
    uint64_t va = jpos.vm - (uint8_t*) prog; \
    idd.addr = va; \
    va2idd[va] = idd; \
    jpos.vm += OFFSET; \
    return; \
}
#define DEFER_JIT_AND_RESERVE_SLOTS(SLOTS) { \
    { \
        deferred_jpos.push_back(jpos); \
        for(size_t i = 0; i < SLOTS; i++) \
            emit_nop(); \
    } \
}


const std::map<x86_64JIT::vm_reg_t, x86_64JIT::arch_reg_t> x86_64JIT::vr2ar = {
    { vm_reg_t::R0 , arch_reg_t::R8  },
    { vm_reg_t::R1 , arch_reg_t::R9  },
    { vm_reg_t::R2 , arch_reg_t::R10 },
    { vm_reg_t::R3 , arch_reg_t::R11 },
    { vm_reg_t::R4 , arch_reg_t::R12 },
    { vm_reg_t::R5 , arch_reg_t::R13 },
    { vm_reg_t::R6 , arch_reg_t::R14 },
    { vm_reg_t::R7 , arch_reg_t::R15 },
    { vm_reg_t::R8 , arch_reg_t::RAX },
    { vm_reg_t::R9 , arch_reg_t::RCX },
    { vm_reg_t::R10, arch_reg_t::RDX },
    { vm_reg_t::R11, arch_reg_t::RBX },
    { vm_reg_t::R12, arch_reg_t::RSI },

    { vm_reg_t::SP , arch_reg_t::RSP }
};


x86_64JIT::x86_64JIT(const void* prog, size_t prog_size, size_t mem_size_mb, bool debug)
: JIT(prog, prog_size, mem_size_mb, debug)
{
    DBG("\ttype 'x86_64 JIT'" << endl);
}


void x86_64JIT::jit()
{
    emit_vm_sub_entry_seq_from_host();
    emit_sys_enter_stub();
    emit_reg_init();

    jit_program();

    emit_vm_exit_syscall_guard();
    emit_mov_reg_reg(RDI, RBP);
    emit_mov_reg_reg(RBP, RDI);
}


void x86_64JIT::jit_program()
{
    DBG("JITing program..." << endl);

    while (jpos.vm < ((uint8_t*) prog) + prog_size) {
        record_addr_mapping();
        jit_vm_instruction();
    }
    jit_deferred();
}


void x86_64JIT::jit_vm_instruction()
{
    static void* instr_jit_handle[] = {
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

    goto *instr_jit_handle[instr(*jpos.vm)];

    _load: {
        idd.dst = reg_dst(*(jpos.vm + 1));
        idd.src = reg_src(*(jpos.vm + 1));
        idd.idx = imm16s(*(jpos.vm + 2));
        emit_mov_reg_b32d(as_arch_reg(idd.dst), as_arch_reg(idd.src), idd.idx);
        JIT_NEXT(+4);
    }

    _store: {
        idd.dst = reg_dst(*(jpos.vm + 1));
        idd.src = reg_src(*(jpos.vm + 1));
        idd.idx = imm16s(*(jpos.vm + 2));
        emit_mov_b32d_reg(as_arch_reg(idd.dst), idd.idx, as_arch_reg(idd.src));
        JIT_NEXT(+4);
    }

    _mov: {
        idd.am = access_mode(*jpos.vm);
        idd.dst = reg_dst(*(jpos.vm + 1));
        switch (idd.am) {
        case REG:
            idd.src = reg_src(*(jpos.vm + 1));
            emit_mov_reg_reg(as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_mov_reg_imm(as_arch_reg(idd.dst), idd.ivs);
            JIT_NEXT(+10);
        }
    }

    _add: {
        idd.am = access_mode(*jpos.vm);
        idd.dst = reg_dst(*(jpos.vm + 1));
        switch (idd.am) {
        case REG:
            idd.src = reg_src(*(jpos.vm + 1));
            emit_add_reg_reg(as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_add_reg_imm64(as_arch_reg(idd.dst), idd.ivs);
            JIT_NEXT(+10);
        }
    }

    _sub: {
        idd.am = access_mode(*jpos.vm);
        idd.dst = reg_dst(*(jpos.vm + 1));
        switch (idd.am) {
        case REG:
            idd.src = reg_src(*(jpos.vm + 1));
            emit_sub_reg_reg(as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_sub_reg_imm64(as_arch_reg(idd.dst), idd.ivs);
            JIT_NEXT(+10);
        }
    }

    _and: {
        idd.am = access_mode(*jpos.vm);
        idd.dst = reg_dst(*(jpos.vm + 1));
        switch (idd.am) {
        case REG:
            idd.src = reg_src(*(jpos.vm + 1));
            emit_and_reg_reg(as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_and_reg_imm64(as_arch_reg(idd.dst), idd.ivs);
            JIT_NEXT(+10);
        }
    }

    _or: {
        idd.am = access_mode(*jpos.vm);
        idd.dst = reg_dst(*(jpos.vm + 1));
        switch (idd.am) {
        case REG:
            idd.src = reg_src(*(jpos.vm + 1));
            emit_or_reg_reg(as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_or_reg_imm64(as_arch_reg(idd.dst), idd.ivs);
            JIT_NEXT(+10);
        }
    }

    _xor: {
        idd.am = access_mode(*jpos.vm);
        idd.dst = reg_dst(*(jpos.vm + 1));
        switch (idd.am) {
        case REG:
            idd.src = reg_src(*(jpos.vm + 1));
            emit_xor_reg_reg(as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_xor_reg_imm64(as_arch_reg(idd.dst), idd.ivs);
            JIT_NEXT(+10);
        }
    }

    _not: {
        idd.dst = reg_dst(*(jpos.vm + 1));
        emit_not_reg(as_arch_reg(idd.dst));
        JIT_NEXT(+2);
    }

    _cmp: {
        idd.am = access_mode(*jpos.vm);
        idd.dst = reg_dst(*(jpos.vm + 1));
        switch (idd.am) {
        case REG:
            idd.src = reg_src(*(jpos.vm + 1));
            emit_cmp_reg_reg(as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_cmp_reg_imm64(as_arch_reg(idd.dst), idd.ivs);
            JIT_NEXT(+10);
        }
    }

    _push: {
        idd.dst = reg_dst(*(jpos.vm + 1));
        emit_push_reg(as_arch_reg(idd.dst));
        JIT_NEXT(+2);
    }

    _pop: {
        idd.dst = reg_dst(*(jpos.vm + 1));
        emit_pop_reg(as_arch_reg(idd.dst));
        JIT_NEXT(+2);
    }

    _call: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_call_imm64(va);
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+13);
        }
        JIT_NEXT(+9);
    }

    _ret: {
        emit_ret();
        JIT_NEXT(+1);
    }

    _jmp: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_jmp_imm64(va);
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+13);
        }
        JIT_NEXT(+9);
    }

    _jmpeq: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_je_imm32((int32_t) ((int64_t) va - (int64_t) jpos.arch));
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+6);
        }
        JIT_NEXT(+9);
    }

    _jmpne: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_jne_imm32((int32_t) ((int64_t) va - (int64_t) jpos.arch));
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+6);
        }
        JIT_NEXT(+9);
    }

    _jmpgt: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_jg_imm32((int32_t) ((int64_t) va - (int64_t) jpos.arch));
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+6);
        }
        JIT_NEXT(+9);
    }

    _jmplt: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_jl_imm32((int32_t) ((int64_t) va - (int64_t) jpos.arch));
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+6);
        }
        JIT_NEXT(+9);
    }

    _jmpge: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_jge_imm32((int32_t) ((int64_t) va - (int64_t) jpos.arch));
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+6);
        }
        JIT_NEXT(+9);
    }

    _jmple: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_jle_imm32((int32_t) ((int64_t) va - (int64_t) jpos.arch));
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+6);
        }
        JIT_NEXT(+9);
    }
}


void x86_64JIT::jit_deferred()
{
    jit_pos_t cur_jpos = jpos;
    std::for_each(deferred_jpos.begin(), deferred_jpos.end(), [this](jit_pos_t& jp) {
        jpos = jp;
        jit_vm_instruction();
    });
    jpos = cur_jpos;
}


void x86_64JIT::emit_vm_sub_entry_seq_from_host()
{
    emit_push_reg(RBP);
    emit_push_reg(RBX);
    emit_push_reg(R12);
    emit_push_reg(R13);
    emit_push_reg(R14);
    emit_push_reg(R15);

    emit_mov_reg_imm(RBP, (uint64_t) &host_sp);
    emit_mov_b8d_reg(RBP, 0, RSP);
}


void x86_64JIT::emit_vm_sub_exit_seq_to_host()
{
    emit_mov_reg_imm(RBP, (uint64_t) &host_sp);
    emit_mov_reg_b8d(RSP, RBP, 0);

    emit_pop_reg(R15);
    emit_pop_reg(R14);
    emit_pop_reg(R13);
    emit_pop_reg(R12);
    emit_pop_reg(RBX);
    emit_pop_reg(RBP);

    emit_ret();
}


void x86_64JIT::emit_non_vm_sub_entry_seq_to_host()
{
    emit_mov_reg_imm(RBP, (uint64_t) &vm_sp);
    emit_mov_b8d_reg(RBP, 0, as_arch_reg(vm_reg_t::SP));

    emit_push_reg(R8);
    emit_push_reg(R9);
    emit_push_reg(R10);
    emit_push_reg(R11);
    emit_push_reg(RAX);
    emit_push_reg(RCX);
    emit_push_reg(RDX);
    emit_push_reg(RSI);
}


void x86_64JIT::emit_non_vm_sub_exit_seq_to_host()
{
    emit_pop_reg(RSI);
    emit_pop_reg(RDX);
    emit_pop_reg(RCX);
    emit_pop_reg(RAX);
    emit_pop_reg(R11);
    emit_pop_reg(R10);
    emit_pop_reg(R9);
    emit_pop_reg(R8);

    emit_mov_reg_imm(RBP, (uint64_t) &vm_sp);
    emit_mov_reg_b8d(as_arch_reg(vm_reg_t::SP), RBP, 0);
}


void x86_64JIT::emit_sys_enter_stub()
{
    uint8_t *pj0, *pn0;
    uint8_t *pj1, *pn1;

    pj0 = jpos.arch;
    jpos.arch += sizeof(JMP_IMM32);
    sys_enter_stub = jpos.arch;
    record_addr_mapping();
    emit_mov_reg_b32d(R8, as_arch_reg(vm_reg_t::SP), 8);
    emit_cmp_reg_imm64(R8, 0);
    pj1 = jpos.arch;
    jpos.arch += sizeof(JE_IMM32);
    emit_non_vm_sub_entry_seq_to_host();
    emit_sys_enter_call();
    emit_non_vm_sub_exit_seq_to_host();
    emit_ret();
    pn1 = jpos.arch;
    emit_add_reg_imm64(RSP, 16);
    emit_vm_reg_save_seq();
    emit_vm_sub_exit_seq_to_host();
    pn0 = jpos.arch;

    jpos.arch = pj0;
    emit_jmp_imm32(pn0 - pj0);
    jpos.arch = pj1;
    emit_je_imm32(pn1 - pj1);

    jpos.vm += 9;
    jpos.arch = pn0;
}


void x86_64JIT::emit_vm_reg_save_seq()
{
    emit_mov_reg_imm(RBP, (uint64_t) reg_dump_area.get());

    emit_mov_b32d_reg(RBP, 0x00, as_arch_reg(vm_reg_t::R0) );
    emit_mov_b32d_reg(RBP, 0x08, as_arch_reg(vm_reg_t::R1) );
    emit_mov_b32d_reg(RBP, 0x10, as_arch_reg(vm_reg_t::R2) );
    emit_mov_b32d_reg(RBP, 0x18, as_arch_reg(vm_reg_t::R3) );
    emit_mov_b32d_reg(RBP, 0x20, as_arch_reg(vm_reg_t::R4) );
    emit_mov_b32d_reg(RBP, 0x28, as_arch_reg(vm_reg_t::R5) );
    emit_mov_b32d_reg(RBP, 0x30, as_arch_reg(vm_reg_t::R6) );
    emit_mov_b32d_reg(RBP, 0x38, as_arch_reg(vm_reg_t::R7) );
    emit_mov_b32d_reg(RBP, 0x40, as_arch_reg(vm_reg_t::R8) );
    emit_mov_b32d_reg(RBP, 0x48, as_arch_reg(vm_reg_t::R9) );
    emit_mov_b32d_reg(RBP, 0x50, as_arch_reg(vm_reg_t::R10));
    emit_mov_b32d_reg(RBP, 0x58, as_arch_reg(vm_reg_t::R11));
    emit_mov_b32d_reg(RBP, 0x60, as_arch_reg(vm_reg_t::R12));
    emit_mov_b32d_reg(RBP, 0x68, as_arch_reg(vm_reg_t::SP) );
}


void x86_64JIT::emit_reg_init()
{
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R0),  0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R1),  0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R2),  0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R3),  0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R4),  0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R5),  0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R6),  0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R7),  0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R8),  0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R9),  0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R10), 0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R11), 0);
    emit_mov_reg_imm32(as_arch_reg(vm_reg_t::R12), 0);

    emit_mov_reg_imm(as_arch_reg(vm_reg_t::SP),  (uint64_t) stack);
}


void x86_64JIT::emit_vm_exit_syscall_guard()
{
    emit_mov_reg_imm32(RBP, 0);
    emit_push_reg(RBP);
    emit_call_imm64((uint64_t) sys_enter_stub);
}


void x86_64JIT::emit_add_reg_imm64(arch_reg_t rd, int64_t imm)
{
    emit_mov_reg_imm(RBP, imm);
    emit_add_reg_reg(rd, RBP);
}


void x86_64JIT::emit_add_reg_reg(arch_reg_t rd, arch_reg_t rs)
{
    *(jpos.arch++) = *(ADD_R_R + 0) | rex_adj_rm(rd, rs);

    rd = reg_base(rd);
    rs = reg_base(rs);

    *(jpos.arch++) = *(ADD_R_R + 1);
    *(jpos.arch++) = MOD_R | (rd << 3) | rs;
}


void x86_64JIT::emit_and_reg_imm64(arch_reg_t rd, int64_t imm)
{
    emit_mov_reg_imm(RBP, imm);
    emit_and_reg_reg(rd, RBP);
}


void x86_64JIT::emit_and_reg_reg(arch_reg_t rd, arch_reg_t rs)
{
    *(jpos.arch++) = *(AND_R_R + 0) | rex_adj_rm(rd, rs);

    rd = reg_base(rd);
    rs = reg_base(rs);

    *(jpos.arch++) = *(AND_R_R + 1);
    *(jpos.arch++) = MOD_R | (rd << 3) | rs;
}


void x86_64JIT::emit_cmp_reg_imm64(arch_reg_t rs, int64_t imm)
{
    emit_mov_reg_imm(RBP, imm);
    emit_cmp_reg_reg(rs, RBP);
}


void x86_64JIT::emit_cmp_reg_reg(arch_reg_t rs1, arch_reg_t rs2)
{
    *(jpos.arch++) = *(CMP_R_R + 0) | rex_adj_rm(rs2, rs1);

    rs1 = reg_base(rs1);
    rs2 = reg_base(rs2);

    *(jpos.arch++) = *(CMP_R_R + 1);
    *(jpos.arch++) = MOD_R | (rs2 << 3) | rs1;
}


void x86_64JIT::emit_or_reg_imm64(arch_reg_t rd, int64_t imm)
{
    emit_mov_reg_imm(RBP, imm);
    emit_or_reg_reg(rd, RBP);
}


void x86_64JIT::emit_or_reg_reg(arch_reg_t rd, arch_reg_t rs)
{
    *(jpos.arch++) = *(OR_R_R + 0) | rex_adj_rm(rd, rs);

    rd = reg_base(rd);
    rs = reg_base(rs);

    *(jpos.arch++) = *(OR_R_R + 1);
    *(jpos.arch++) = MOD_R | (rd << 3) | rs;
}


void x86_64JIT::emit_not_reg(arch_reg_t r)
{
    *(jpos.arch++) = *(NOT_R + 0) | rex_adj_m(r);

    r = reg_base(r);

    *(jpos.arch++) = *(NOT_R + 1);
    *(jpos.arch++) = MOD_R | (0b010 << 3) | r;
}


void x86_64JIT::emit_sub_reg_imm64(arch_reg_t rd, int64_t imm)
{
    emit_mov_reg_imm(RBP, imm);
    emit_sub_reg_reg(rd, RBP);
}


void x86_64JIT::emit_sub_reg_reg(arch_reg_t rd, arch_reg_t rs)
{
    *(jpos.arch++) = *(SUB_R_R + 0) | rex_adj_rm(rd, rs);

    rd = reg_base(rd);
    rs = reg_base(rs);

    *(jpos.arch++) = *(SUB_R_R + 1);
    *(jpos.arch++) = MOD_R | (rd << 3) | rs;
}


void x86_64JIT::emit_xor_reg_imm64(arch_reg_t rd, int64_t imm)
{
    emit_mov_reg_imm(RBP, imm);
    emit_xor_reg_reg(rd, RBP);
}


void x86_64JIT::emit_xor_reg_reg(arch_reg_t rd, arch_reg_t rs)
{
    *(jpos.arch++) = *(XOR_R_R + 0) | rex_adj_rm(rd, rs);

    rd = reg_base(rd);
    rs = reg_base(rs);

    *(jpos.arch++) = *(XOR_R_R + 1);
    *(jpos.arch++) = MOD_R | (rd << 3) | rs;
}


void x86_64JIT::emit_mov_b8d_reg(arch_reg_t rb, int8_t d, arch_reg_t rs)
{
    *(jpos.arch++) = *(MOV_BD_R + 0) | rex_adj_rm(rs, rb);

    rs = reg_base(rs);
    rb = reg_base(rb);

    *(jpos.arch++) = *(MOV_BD_R + 1);
    *(jpos.arch++) = MOD_B8D | (rs << 3) | rb;
    if (rb == RSP || rb == R12) {
        *(jpos.arch++) = (0b100 << 3) | rb;
    }
    *(jpos.arch++) = d;
}


void x86_64JIT::emit_mov_reg_b8d(arch_reg_t rd, arch_reg_t rb, int8_t d)
{
    *(jpos.arch++) = *(MOV_R_BD + 0) | rex_adj_rm(rd, rb);

    rd = reg_base(rd);
    rb = reg_base(rb);

    *(jpos.arch++) = *(MOV_R_BD + 1);
    *(jpos.arch++) = MOD_B8D | (rd << 3) | rb;
    if (rb == RSP || rb == R12) {
        *(jpos.arch++) = (0b100 << 3) | rb;
    }
    *(jpos.arch++) = d;
}


void x86_64JIT::emit_mov_b32d_reg(arch_reg_t rb, int32_t d, arch_reg_t rs)
{
    *(jpos.arch++) = *(MOV_BD_R + 0) | rex_adj_rm(rs, rb);

    rs = reg_base(rs);
    rb = reg_base(rb);

    *(jpos.arch++) = *(MOV_BD_R + 1);
    *(jpos.arch++) = MOD_B32D | (rs << 3) | rb;
    if (rb == RSP || rb == R12) {
        *(jpos.arch++) = (0b100 << 3) | rb;
    }
    *((int32_t*) jpos.arch) = d;
    jpos.arch += 4;
}


void x86_64JIT::emit_mov_reg_b32d(arch_reg_t rd, arch_reg_t rb, int32_t d)
{
    *(jpos.arch++) = *(MOV_R_BD + 0) | rex_adj_rm(rd, rb);

    rd = reg_base(rd);
    rb = reg_base(rb);

    *(jpos.arch++) = *(MOV_R_BD + 1);
    *(jpos.arch++) = MOD_B32D | (rd << 3) | rb;
    if (rb == RSP || rb == R12) {
        *(jpos.arch++) = (0b100 << 3) | rb;
    }
    *((int32_t*) jpos.arch) = d;
    jpos.arch += 4;
}


void x86_64JIT::emit_mov_reg_imm(arch_reg_t rd, int64_t imm)
{
    int32_t upper_dword = (imm & 0xffffffff00000000) >> 32;
    if (upper_dword == 0 || upper_dword == -1) {
        emit_mov_reg_imm32(rd, (int32_t) imm);
    } else {
        emit_mov_reg_imm64(rd, imm);
    }
}


void x86_64JIT::emit_mov_reg_imm32(arch_reg_t rd, int32_t imm)
{
    *(jpos.arch++) = *(MOV_R_IMM32 + 0) | rex_adj_m(rd);

    rd = reg_base(rd);

    *(jpos.arch++) = *(MOV_R_IMM32 + 1);
    *(jpos.arch++) = MOD_R | rd;
    *((int32_t*) jpos.arch) = imm;
    jpos.arch += 4;
}


void x86_64JIT::emit_mov_reg_imm64(arch_reg_t rd, int64_t imm)
{
    *(jpos.arch++) = *(MOV_R_IMM64 + 0) | rex_adj_m(rd);

    rd = reg_base(rd);

    *(jpos.arch++) = *(MOV_R_IMM64 + 1) | rd;
    *((int64_t*) jpos.arch) = imm;
    jpos.arch += 8;
}


void x86_64JIT::emit_mov_reg_reg(arch_reg_t rd, arch_reg_t rs)
{
    *(jpos.arch++) = *(MOV_R_R + 0) | rex_adj_rm(rd, rs);

    rd = reg_base(rd);
    rs = reg_base(rs);

    *(jpos.arch++) = *(MOV_R_R + 1);
    *(jpos.arch++) = MOD_R | (rd << 3) | rs;
}


void x86_64JIT::emit_call_imm64(uint64_t imm)
{
    emit_mov_reg_imm(RBP, imm);
    emit_call_reg(RBP);
}


void x86_64JIT::emit_call_reg(arch_reg_t rs)
{
    *(jpos.arch++) = *(CALL_R + 0) | rex_adj_m(rs);

    rs = reg_base(rs);

    *(jpos.arch++) = *(CALL_R + 1);
    *(jpos.arch++) = MOD_R | (0b010 << 3) | rs;
}


void x86_64JIT::emit_je_imm32(int32_t imm)
{
    *(jpos.arch++) = *(JE_IMM32 + 0);
    *(jpos.arch++) = *(JE_IMM32 + 1);
    *((int32_t*) jpos.arch) = imm - sizeof(JE_IMM32);
    jpos.arch += 4;
}


void x86_64JIT::emit_jne_imm32(int32_t imm)
{
    *(jpos.arch++) = *(JNE_IMM32 + 0);
    *(jpos.arch++) = *(JNE_IMM32 + 1);
    *((int32_t*) jpos.arch) = imm - sizeof(JNE_IMM32);
    jpos.arch += 4;
}


void x86_64JIT::emit_jg_imm32(int32_t imm)
{
    *(jpos.arch++) = *(JG_IMM32 + 0);
    *(jpos.arch++) = *(JG_IMM32 + 1);
    *((int32_t*) jpos.arch) = imm - sizeof(JG_IMM32);
    jpos.arch += 4;
}


void x86_64JIT::emit_jge_imm32(int32_t imm)
{
    *(jpos.arch++) = *(JGE_IMM32 + 0);
    *(jpos.arch++) = *(JGE_IMM32 + 1);
    *((int32_t*) jpos.arch) = imm - sizeof(JGE_IMM32);
    jpos.arch += 4;
}


void x86_64JIT::emit_jl_imm32(int32_t imm)
{
    *(jpos.arch++) = *(JL_IMM32 + 0);
    *(jpos.arch++) = *(JL_IMM32 + 1);
    *((int32_t*) jpos.arch) = imm - sizeof(JL_IMM32);
    jpos.arch += 4;
}


void x86_64JIT::emit_jle_imm32(int32_t imm)
{
    *(jpos.arch++) = *(JLE_IMM32 + 0);
    *(jpos.arch++) = *(JLE_IMM32 + 1);
    *((int32_t*) jpos.arch) = imm - sizeof(JLE_IMM32);
    jpos.arch += 4;
}


void x86_64JIT::emit_jmp_imm32(int32_t imm)
{
    *(jpos.arch++) = *(JMP_IMM32 + 0);
    *((int32_t*) jpos.arch) = imm - sizeof(JMP_IMM32);
    jpos.arch += 4;
}


void x86_64JIT::emit_jmp_imm64(uint64_t imm)
{
    emit_mov_reg_imm(RBP, imm);
    emit_jmp_reg(RBP);
}


void x86_64JIT::emit_jmp_reg(arch_reg_t rs)
{
    *(jpos.arch++) = *(JMP_R + 0) | rex_adj_m(rs);

    rs = reg_base(rs);

    *(jpos.arch++) = *(JMP_R + 1);
    *(jpos.arch++) = MOD_R | (0b100 << 3) | rs;
}


void x86_64JIT::emit_nop()
{
    *(jpos.arch++) = *(NOP + 0);
}


void x86_64JIT::emit_pop_reg(arch_reg_t rd)
{
    if (reg_base(rd) != rd) {
        *(jpos.arch++) = *(POP_REG + 0);
    }

    rd = reg_base(rd);

    *(jpos.arch++) = *(POP_REG + 1) | rd;
}


void x86_64JIT::emit_push_reg(arch_reg_t rs)
{
    if (reg_base(rs) != rs) {
        *(jpos.arch++) = *(PUSH_REG + 0);
    }

    rs = reg_base(rs);

    *(jpos.arch++) = *(PUSH_REG + 1) | rs;
}


void x86_64JIT::emit_ret()
{
    *(jpos.arch++) = *(RET + 0);
}


void x86_64JIT::emit_sys_enter_call()
{
    emit_mov_reg_imm(RBP, (uint64_t) &vm_sp);
    emit_mov_reg_b8d(RDI, RBP, 0);

    emit_call_imm64((uint64_t) sys_enter);

    emit_mov_reg_imm(RBP, (uint64_t) &vm_sp);
    emit_mov_b8d_reg(RBP, 0, RAX);
}
