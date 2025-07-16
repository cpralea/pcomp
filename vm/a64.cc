#include <algorithm>
#include <map>

#include "exe.h"
#include "a64.h"


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


const std::map<AArch64JIT::vm_reg_t, AArch64JIT::arch_reg_t> AArch64JIT::vr2ar = {
    { vm_reg_t::R0 , arch_reg_t::R27 },
    { vm_reg_t::R1 , arch_reg_t::R26 },
    { vm_reg_t::R2 , arch_reg_t::R25 },
    { vm_reg_t::R3 , arch_reg_t::R24 },
    { vm_reg_t::R4 , arch_reg_t::R23 },
    { vm_reg_t::R5 , arch_reg_t::R22 },
    { vm_reg_t::R6 , arch_reg_t::R21 },
    { vm_reg_t::R7 , arch_reg_t::R20 },
    { vm_reg_t::R8 , arch_reg_t::R19 },
    { vm_reg_t::R9 , arch_reg_t::R15 },
    { vm_reg_t::R10, arch_reg_t::R14 },
    { vm_reg_t::R11, arch_reg_t::R13 },
    { vm_reg_t::R12, arch_reg_t::R12 },

    { vm_reg_t::SP , arch_reg_t::R28 }
};


AArch64JIT::AArch64JIT(const void* prog, size_t prog_size, size_t mem_size_mb, bool debug)
: JIT(prog, prog_size, mem_size_mb, debug)
{
    DBG("\ttype 'AArch64 JIT'" << endl);
}


void AArch64JIT::jit()
{
#ifdef __APPLE__
    pthread_jit_write_protect_np(false);
#endif

    emit_vm_sub_entry_seq_from_host();
    emit_sys_enter_stub();
    emit_reg_init();

    jit_program();
    
    emit_vm_exit_syscall_guard();

#ifdef __APPLE__
    pthread_jit_write_protect_np(true);
#endif
}


void AArch64JIT::jit_program()
{
    DBG("JITing program..." << endl);

    while (jpos.vm < ((uint8_t*) prog) + prog_size) {
        record_addr_mapping();
        jit_vm_instruction();
    }
    jit_deferred();
}


void AArch64JIT::jit_vm_instruction()
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
        emit_mov_reg_imm(R11, idd.idx);
        emit_adds_ereg(R11, as_arch_reg(idd.src), R11);
        emit_ldr_unsigned_offset(as_arch_reg(idd.dst), R11, 0);
        JIT_NEXT(+4);
    }

    _store: {
        idd.dst = reg_dst(*(jpos.vm + 1));
        idd.src = reg_src(*(jpos.vm + 1));
        idd.idx = imm16s(*(jpos.vm + 2));
        emit_mov_reg_imm(R11, idd.idx);
        emit_adds_ereg(R11, as_arch_reg(idd.dst), R11);
        emit_str_unsigned_offset(as_arch_reg(idd.src), R11, 0);
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
            emit_adds_ereg(as_arch_reg(idd.dst), as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_mov_reg_imm(R11, idd.ivs);
            emit_adds_ereg(as_arch_reg(idd.dst), as_arch_reg(idd.dst), R11);
            JIT_NEXT(+10);
        }
    }

    _sub: {
        idd.am = access_mode(*jpos.vm);
        idd.dst = reg_dst(*(jpos.vm + 1));
        switch (idd.am) {
        case REG:
            idd.src = reg_src(*(jpos.vm + 1));
            emit_subs_ereg(as_arch_reg(idd.dst), as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_mov_reg_imm(R11, idd.ivs);
            emit_subs_ereg(as_arch_reg(idd.dst), as_arch_reg(idd.dst), R11);
            JIT_NEXT(+10);
        }
    }

    _and: {
        idd.am = access_mode(*jpos.vm);
        idd.dst = reg_dst(*(jpos.vm + 1));
        switch (idd.am) {
        case REG:
            idd.src = reg_src(*(jpos.vm + 1));
            emit_and_sreg(as_arch_reg(idd.dst), as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_mov_reg_imm(R11, idd.ivs);
            emit_and_sreg(as_arch_reg(idd.dst), as_arch_reg(idd.dst), R11);
            JIT_NEXT(+10);
        }
    }

    _or: {
        idd.am = access_mode(*jpos.vm);
        idd.dst = reg_dst(*(jpos.vm + 1));
        switch (idd.am) {
        case REG:
            idd.src = reg_src(*(jpos.vm + 1));
            emit_orr_sreg(as_arch_reg(idd.dst), as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_mov_reg_imm(R11, idd.ivs);
            emit_orr_sreg(as_arch_reg(idd.dst), as_arch_reg(idd.dst), R11);
            JIT_NEXT(+10);
        }
    }

    _xor: {
        idd.am = access_mode(*jpos.vm);
        idd.dst = reg_dst(*(jpos.vm + 1));
        switch (idd.am) {
        case REG:
            idd.src = reg_src(*(jpos.vm + 1));
            emit_eor_sreg(as_arch_reg(idd.dst), as_arch_reg(idd.dst), as_arch_reg(idd.src));
            JIT_NEXT(+2);
        case IMM:
            idd.ivs = imm64s(*(jpos.vm + 2));
            emit_mov_reg_imm(R11, idd.ivs);
            emit_eor_sreg(as_arch_reg(idd.dst), as_arch_reg(idd.dst), R11);
            JIT_NEXT(+10);
        }
    }

    _not: {
        idd.dst = reg_dst(*(jpos.vm + 1));
        emit_orn_sreg(as_arch_reg(idd.dst), ZR, as_arch_reg(idd.dst));
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
            emit_cmp_reg_imm(as_arch_reg(idd.dst), idd.ivs);
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
            emit_adr(R11, +7 * 4);
            emit_push_reg(R11);
            uint8_t *target_jpos_arch = jpos.arch + 4 * 4;
            emit_mov_reg_imm(R11, va);
            while (jpos.arch < target_jpos_arch)
                emit_nop();
            emit_br(R11);
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+7);
        }
        JIT_NEXT(+9);
    }

    _ret: {
        emit_pop_reg(LR);
        emit_ret();
        JIT_NEXT(+1);
    }

    _jmp: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_mov_reg_imm(R11, va);
            emit_br(R11);
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+5);
        }
        JIT_NEXT(+9);
    }

    _jmpeq: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_b_cond(EQ, (uint32_t*) va - (uint32_t*) jpos.arch);
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+1);
        }
        JIT_NEXT(+9);
    }

    _jmpne: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_b_cond(NE, (uint32_t*) va - (uint32_t*) jpos.arch);
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+1);
        }
        JIT_NEXT(+9);
    }

    _jmpgt: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_b_cond(GT, (uint32_t*) va - (uint32_t*) jpos.arch);
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+1);
        }
        JIT_NEXT(+9);
    }

    _jmplt: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_b_cond(LT, (uint32_t*) va - (uint32_t*) jpos.arch);
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+1);
        }
        JIT_NEXT(+9);
    }

    _jmpge: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_b_cond(GE, (uint32_t*) va - (uint32_t*) jpos.arch);
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+1);
        }
        JIT_NEXT(+9);
    }

    _jmple: {
        idd.ivu = imm64u(*(jpos.vm + 1));
        uint64_t va = as_arch_addr(idd.ivu);
        if (va != (uint64_t) -1) {
            emit_b_cond(LE, (uint32_t*) va - (uint32_t*) jpos.arch);
        }
        else {
            DEFER_JIT_AND_RESERVE_SLOTS(+1);
        }
        JIT_NEXT(+9);
    }
}


void AArch64JIT::jit_deferred()
{
    jit_pos_t cur_jpos = jpos;
    std::for_each(deferred_jpos.begin(), deferred_jpos.end(), [this](jit_pos_t& jp) {
        jpos = jp;
        jit_vm_instruction();
    });
    jpos = cur_jpos;
}


void AArch64JIT::emit_vm_sub_entry_seq_from_host()
{
    emit_stp_pre_idx(FP, LR, SP, -16);
    emit_mov_reg_sp(FP, SP);

    emit_stp_pre_idx(R19, R20, SP, -16);
    emit_stp_pre_idx(R21, R22, SP, -16);
    emit_stp_pre_idx(R23, R24, SP, -16);
    emit_stp_pre_idx(R25, R26, SP, -16);
    emit_stp_pre_idx(R27, R28, SP, -16);
}


void AArch64JIT::emit_vm_sub_exit_seq_to_host()
{
    emit_ldp_post_idx(R27, R28, SP, 16);
    emit_ldp_post_idx(R25, R26, SP, 16);
    emit_ldp_post_idx(R23, R24, SP, 16);
    emit_ldp_post_idx(R21, R22, SP, 16);
    emit_ldp_post_idx(R19, R20, SP, 16);

    emit_ldp_post_idx(FP, LR, SP, 16);
    emit_ret();
}


void AArch64JIT::emit_non_vm_sub_entry_seq_to_host()
{
    emit_stp_pre_idx(R14, R15, SP, -16);
    emit_stp_pre_idx(R12, R13, SP, -16);
}


void AArch64JIT::emit_non_vm_sub_exit_seq_to_host()
{
    emit_ldp_post_idx(R12, R13, SP, 16);
    emit_ldp_post_idx(R14, R15, SP, 16);
}


void AArch64JIT::emit_sys_enter_stub()
{
    uint8_t *pj0, *pn0;
    uint8_t *pj1, *pn1;

    pj0 = jpos.arch;
    jpos.arch += 4;
    sys_enter_stub = jpos.arch;
    record_addr_mapping();
    emit_ldr_unsigned_offset(R27, as_arch_reg(vm_reg_t::SP), 1);
    emit_cmp_reg_imm(R27, 0);
    pj1 = jpos.arch;
    jpos.arch += 4;
    emit_non_vm_sub_entry_seq_to_host();
    emit_sys_enter_call();
    emit_non_vm_sub_exit_seq_to_host();
    emit_pop_reg(LR);
    emit_ret();
    pn1 = jpos.arch;
    emit_add(as_arch_reg(vm_reg_t::SP), as_arch_reg(vm_reg_t::SP), 16);
    emit_vm_reg_save_seq();
    emit_vm_sub_exit_seq_to_host();
    pn0 = jpos.arch;

    jpos.arch = pj0;
    emit_b((uint32_t*) pn0 - (uint32_t*) pj0);
    jpos.arch = pj1;
    emit_b_cond(EQ, (uint32_t*) pn1 - (uint32_t*) pj1);

    jpos.vm += 9;
    jpos.arch = pn0;
}


void AArch64JIT::emit_vm_reg_save_seq()
{
    emit_mov_reg_imm(R11, (uint64_t) reg_dump_area.get());

    emit_str_post_idx(as_arch_reg(vm_reg_t::R0),  R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R1),  R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R2),  R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R3),  R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R4),  R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R5),  R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R6),  R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R7),  R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R8),  R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R9),  R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R10), R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R11), R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::R12), R11, 8);
    emit_str_post_idx(as_arch_reg(vm_reg_t::SP),  R11, 8);
}


void AArch64JIT::emit_reg_init()
{
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R0),  0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R1),  0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R2),  0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R3),  0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R4),  0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R5),  0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R6),  0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R7),  0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R8),  0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R9),  0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R10), 0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R11), 0);
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R12), 0);

    emit_mov_reg_imm(as_arch_reg(vm_reg_t::SP),  (uint64_t) stack);
}


void AArch64JIT::emit_vm_exit_syscall_guard()
{
    emit_mov_reg_imm(as_arch_reg(vm_reg_t::R0), 0);
    emit_push_reg(as_arch_reg(vm_reg_t::R0));

    emit_adr(R11, 12);
    emit_push_reg(R11);
    emit_b(-((uint32_t*) jpos.arch - (uint32_t*) sys_enter_stub));
}


void AArch64JIT::emit_add(arch_reg_t rd, arch_reg_t rs, uint16_t imm)
{
    *((uint32_t*) jpos.arch)    = ADD_IMM
                                | ((imm & 0b0000111111111111) << 10)
                                | (rs << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_adds_ereg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2)
{
    *((uint32_t*) jpos.arch)    = ADDS_EREG
                                | (rs2 << 16)
                                | (SXTX << 13)
                                | (rs1 << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_adr(arch_reg_t rd, int32_t imm)
{
    uint32_t imm_hi = (imm & 0b00000000000111111111111111111100) >> 2;
    uint32_t imm_lo = (imm & 0b00000000000000000000000000000011);

    *((uint32_t*) jpos.arch)    = ADR
                                | (imm_lo << 29)
                                | (imm_hi << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_and_sreg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2)
{
    *((uint32_t*) jpos.arch)    = AND_SREG
                                | (rs2 << 16)
                                | (rs1 << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_eor_sreg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2)
{
    *((uint32_t*) jpos.arch)    = EOR_SREG
                                | (rs2 << 16)
                                | (rs1 << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_movk(arch_reg_t rd, uint8_t shift, int16_t imm)
{
    *((uint32_t*) jpos.arch)    = MOVK
                                | (((shift >> 4) & 0b00000011) << 21)
                                | (((uint16_t) imm) << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_movz(arch_reg_t rd, uint8_t shift, int16_t imm)
{
    *((uint32_t*) jpos.arch)    = MOVZ
                                | (((shift >> 4) & 0b00000011) << 21)
                                | (((uint16_t) imm) << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_orn_sreg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2)
{
    *((uint32_t*) jpos.arch)    = ORN_SREG
                                | (rs2 << 16)
                                | (rs1 << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_orr_sreg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2)
{
    *((uint32_t*) jpos.arch)    = ORR_SREG
                                | (rs2 << 16)
                                | (rs1 << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_subs_ereg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2)
{
    *((uint32_t*) jpos.arch)    = SUBS_EREG
                                | (rs2 << 16)
                                | (SXTX << 13)
                                | (rs1 << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_ldp_post_idx(arch_reg_t rd1, arch_reg_t rd2, arch_reg_t rb, int32_t imm)
{
    *((uint32_t*) jpos.arch)    = LDP_POST_IDX
                                | (((imm >> 3) & 0b01111111) << 15)
                                | (rd2 << 10)
                                | (rb << 5)
                                | rd1;
    jpos.arch += 4;
}


void AArch64JIT::emit_ldr_post_idx(arch_reg_t rd, arch_reg_t rb, int32_t imm)
{
    *((uint32_t*) jpos.arch)    = LDR_POST_IDX
                                | ((imm & 0b0000000111111111) << 12)
                                | (rb << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_ldr_unsigned_offset(arch_reg_t rd, arch_reg_t rb, uint16_t imm)
{
    *((uint32_t*) jpos.arch)    = LDR_UNSIGNED_OFFSET
                                | ((imm & 0b0000111111111111) << 10)
                                | (rb << 5)
                                | rd;
    jpos.arch += 4;
}


void AArch64JIT::emit_stp_pre_idx(arch_reg_t rs1, arch_reg_t rs2, arch_reg_t rb, int32_t imm)
{
    *((uint32_t*) jpos.arch)    = STP_PRE_IDX
                                | (((imm >> 3) & 0b01111111) << 15)
                                | (rs2 << 10)
                                | (rb << 5)
                                | rs1;
    jpos.arch += 4;
}


void AArch64JIT::emit_str_post_idx(arch_reg_t rs, arch_reg_t rb, int16_t imm)
{
    *((uint32_t*) jpos.arch)    = STR_POST_IDX
                                | ((imm & 0b0000000111111111) << 12)
                                | (rb << 5)
                                | rs;
    jpos.arch += 4;
}


void AArch64JIT::emit_str_pre_idx(arch_reg_t rs, arch_reg_t rb, int16_t imm)
{
    *((uint32_t*) jpos.arch)    = STR_PRE_IDX
                                | ((imm & 0b0000000111111111) << 12)
                                | (rb << 5)
                                | rs;
    jpos.arch += 4;
}


void AArch64JIT::emit_str_unsigned_offset(arch_reg_t rs, arch_reg_t rb, uint16_t imm)
{
    *((uint32_t*) jpos.arch)    = STR_UNSIGNED_OFFSET
                                | ((imm & 0b0000111111111111) << 10)
                                | (rb << 5)
                                | rs;
    jpos.arch += 4;
}


void AArch64JIT::emit_b(int32_t imm)
{
    *((uint32_t*) jpos.arch)    = B
                                | (imm & 0b00000011111111111111111111111111);
    jpos.arch += 4;
}


void AArch64JIT::emit_b_cond(arch_cond_t cond, int32_t imm)
{
    *((uint32_t*) jpos.arch)    = B_COND
                                | ((imm & 0b00000000000001111111111111111111) << 5)
                                | cond;
    jpos.arch += 4;
}


void AArch64JIT::emit_bl(int32_t imm)
{
    *((uint32_t*) jpos.arch)    = BL
                                | (imm & 0b00000011111111111111111111111111);
    jpos.arch += 4;
}


void AArch64JIT::emit_blr(arch_reg_t reg)
{
    *((uint32_t*) jpos.arch)    = BLR
                                | (reg << 5);
    jpos.arch += 4;
}


void AArch64JIT::emit_br(arch_reg_t reg)
{
    *((uint32_t*) jpos.arch)    = BR
                                | (reg << 5);
    jpos.arch += 4;
}


void AArch64JIT::emit_nop()
{
    *((uint32_t*) jpos.arch)    = NOP;
    jpos.arch += 4;
}


void AArch64JIT::emit_ret(arch_reg_t reg)
{
    *((uint32_t*) jpos.arch)    = RET
                                | (reg << 5);
    jpos.arch += 4;
}


void AArch64JIT::emit_cmp_reg_imm(arch_reg_t rs, int64_t imm)
{
    emit_mov_reg_imm(R11, imm);
    emit_subs_ereg(ZR, rs, R11);
}


void AArch64JIT::emit_cmp_reg_reg(arch_reg_t rs1, arch_reg_t rs2)
{
    emit_subs_ereg(ZR, rs1, rs2);
}


void AArch64JIT::emit_mov_reg_imm(arch_reg_t rd, int64_t imm)
{
    int16_t partial_imm;

    partial_imm = imm & 0x000000000000ffff;
    emit_movz(rd, 0, partial_imm);
    imm >>= 16;
    if (imm == 0)
        return;

    partial_imm = imm & 0x000000000000ffff;
    emit_movk(rd, 16, partial_imm);
    imm >>= 16;
    if (imm == 0)
        return;

    partial_imm = imm & 0x000000000000ffff;
    emit_movk(rd, 32, partial_imm);
    imm >>= 16;
    if (imm == 0)
        return;

    partial_imm = imm & 0x000000000000ffff;
    emit_movk(rd, 48, partial_imm);
}


void AArch64JIT::emit_mov_reg_reg(arch_reg_t rd, arch_reg_t rs)
{
    emit_orr_sreg(rd, ZR, rs);
}


void AArch64JIT::emit_mov_reg_sp(arch_reg_t rd, arch_reg_t rs)
{
    emit_add(rd, rs, 0);
}


void AArch64JIT::emit_pop_reg(arch_reg_t rd)
{
    emit_ldr_post_idx(rd, as_arch_reg(vm_reg_t::SP), 8);
}


void AArch64JIT::emit_push_reg(arch_reg_t rs)
{
    emit_str_pre_idx(rs, as_arch_reg(vm_reg_t::SP), -8);
}


void AArch64JIT::emit_ret()
{
    emit_ret(LR);
}


void AArch64JIT::emit_sys_enter_call()
{
    emit_mov_reg_reg(R0, as_arch_reg(vm_reg_t::SP));
    emit_mov_reg_imm(R11, (uint64_t) sys_enter);
    emit_blr(R11);
    emit_mov_reg_reg(as_arch_reg(vm_reg_t::SP), R0);
}
