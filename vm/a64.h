#pragma once


#include <map>

#include "jit.h"


class AArch64JIT final : public JIT {
public:
    AArch64JIT(const void* prog, size_t prog_size, size_t mem_size_mb, bool debug);

private:
    static constexpr const char* OBJDUMP_FMT        = "objdump -b binary -m aarch64 --adjust-vma 0x%llx -D %s > %s";
    const char* get_objdump_fmt() const override    { return OBJDUMP_FMT; }

    void jit() override;

    typedef enum : uint8_t {
        // General-purpose Registers
        R0                                          = 0b00000000,
        R1                                          = 0b00000001,
        R2                                          = 0b00000010,
        R3                                          = 0b00000011,
        R4                                          = 0b00000100,
        R5                                          = 0b00000101,
        R6                                          = 0b00000110,
        R7                                          = 0b00000111,
        R8                                          = 0b00001000,
        R9                                          = 0b00001001,
        R10                                         = 0b00001010,
        R11                                         = 0b00001011,
        R12                                         = 0b00001100,
        R13                                         = 0b00001101,
        R14                                         = 0b00001110,
        R15                                         = 0b00001111,
        R16                                         = 0b00010000,
        R17                                         = 0b00010001,
        R18                                         = 0b00010010,
        R19                                         = 0b00010011,
        R20                                         = 0b00010100,
        R21                                         = 0b00010101,
        R22                                         = 0b00010110,
        R23                                         = 0b00010111,
        R24                                         = 0b00011000,
        R25                                         = 0b00011001,
        R26                                         = 0b00011010,
        R27                                         = 0b00011011,
        R28                                         = 0b00011100,
        R29                                         = 0b00011101,
        R30                                         = 0b00011110,
        R31                                         = 0b00011111,

        // Aliases
        FP                                          = R29,
        LR                                          = R30,
        ZR                                          = R31,
        SP                                          = R31,
    } arch_reg_t;

    static const std::map<vm_reg_t, arch_reg_t> vr2ar;

    typedef enum : uint32_t {
        // Data Processing -- Immediate
        DG0_DP_IMM                                  = 0b00010000000000000000000000000000,
            // PC-rel. addressing
            DG0_DP_IMM_DG1_PC_REL_ADDR              = 0b00000000000000000000000000000000,
            // Add/subtract (immediate)
            DG0_DP_IMM_DG1_ADD_SUB_IMM              = 0b00000001000000000000000000000000,
            // Move wide (immediate)
            DG0_DP_IMM_DG1_MOV_WIDE_IMM             = 0b00000010100000000000000000000000,

        // Data Processing -- Register
        DG0_DP_REG                                  = 0b00001010000000000000000000000000,
            // Logical (shifted register)
            DG0_DP_REG_DG1_LOGICAL_SREG             = 0b00000000000000000000000000000000,
            // Add/subtract (extended register)
            DG0_DP_REG_DG1_ADD_SUB_EREG             = 0b00000001001000000000000000000000,

        // Branches, Exception Generating and System instructions
        DG0_BR_EG_SYS                               = 0b00010100000000000000000000000000,
            // Conditional branch (immediate)
            DG0_BR_EG_SYS_DG1_CBR_IMM               = 0b01000000000000000000000000000000,
            // Hints
            DG0_BR_EG_SYS_DG1_HINT                  = 0b11000001000000110010000000011111,
            // Unconditional branch (register)
            DG0_BR_EG_SYS_DG1_UBR_R                 = 0b11000010000000000000000000000000,
            // Unconditional branch (immediate)
            DG0_BR_EG_SYS_DG1_UBR_IMM               = 0b00000000000000000000000000000000,

        // Loads and Stores
        DG0_LS                                      = 0b00001000000000000000000000000000,
            // Load/store register pair (pre-indexed)
            DG0_LS_DG1_LSRP_PRE_IDX                 = 0b00101001100000000000000000000000,
            // Load/store register pair (post-indexed)
            DG0_LS_DG1_LSRP_POST_IDX                = 0b10101000100000000000000000000000,
            // Load/store register (immediate post-indexed)
            DG0_LS_DG1_LSR_POST_IDX                 = 0b00110000000000000000010000000000,
            // Load/store register (immediate pre-indexed)
            DG0_LS_DG1_LSR_PRE_IDX                  = 0b00110000000000000000110000000000,
            // Load/store register (unsigned immediate)
            DG0_LS_DG1_LSR_UNSIGNED_IMM             = 0b00110001000000000000000000000000,
    } arch_dg_t;

    typedef enum : uint32_t {
        ADD_IMM                                     = DG0_DP_IMM
                                                    | DG0_DP_IMM_DG1_ADD_SUB_IMM
                                                    | 0b10000000000000000000000000000000,
        ADDS_EREG                                   = DG0_DP_REG
                                                    | DG0_DP_REG_DG1_ADD_SUB_EREG
                                                    | 0b10100000000000000000000000000000,
        ADR                                         = DG0_DP_IMM
                                                    | DG0_DP_IMM_DG1_PC_REL_ADDR
                                                    | 0b00000000000000000000000000000000,
        AND_SREG                                    = DG0_DP_REG
                                                    | DG0_DP_REG_DG1_LOGICAL_SREG
                                                    | 0b10000000000000000000000000000000,
        B                                           = DG0_BR_EG_SYS
                                                    | DG0_BR_EG_SYS_DG1_UBR_IMM
                                                    | 0b00000000000000000000000000000000,
        B_COND                                      = DG0_BR_EG_SYS
                                                    | DG0_BR_EG_SYS_DG1_CBR_IMM
                                                    | 0b00000000000000000000000000000000,
        BL                                          = DG0_BR_EG_SYS
                                                    | DG0_BR_EG_SYS_DG1_UBR_IMM
                                                    | 0b10000000000000000000000000000000,
        BLR                                         = DG0_BR_EG_SYS
                                                    | DG0_BR_EG_SYS_DG1_UBR_R
                                                    | 0b00000000001111110000000000000000,
        BR                                          = DG0_BR_EG_SYS
                                                    | DG0_BR_EG_SYS_DG1_UBR_R
                                                    | 0b00000000000111110000000000000000,
        EOR_SREG                                    = DG0_DP_REG
                                                    | DG0_DP_REG_DG1_LOGICAL_SREG
                                                    | 0b11000000000000000000000000000000,
        LDP_POST_IDX                                = DG0_LS
                                                    | DG0_LS_DG1_LSRP_POST_IDX
                                                    | 0b10000000010000000000000000000000,
        LDR_POST_IDX                                = DG0_LS
                                                    | DG0_LS_DG1_LSR_POST_IDX
                                                    | 0b11000000010000000000000000000000,
        LDR_UNSIGNED_OFFSET                         = DG0_LS
                                                    | DG0_LS_DG1_LSR_UNSIGNED_IMM
                                                    | 0b11000000010000000000000000000000,
        MOVK                                        = DG0_DP_IMM
                                                    | DG0_DP_IMM_DG1_MOV_WIDE_IMM
                                                    | 0b11100000000000000000000000000000,
        MOVZ                                        = DG0_DP_IMM
                                                    | DG0_DP_IMM_DG1_MOV_WIDE_IMM
                                                    | 0b11000000000000000000000000000000,
        NOP                                         = DG0_BR_EG_SYS
                                                    | DG0_BR_EG_SYS_DG1_HINT
                                                    | 0b00000000000000000000000000000000,
        ORN_SREG                                    = DG0_DP_REG
                                                    | DG0_DP_REG_DG1_LOGICAL_SREG
                                                    | 0b10100000001000000000000000000000,
        ORR_SREG                                    = DG0_DP_REG
                                                    | DG0_DP_REG_DG1_LOGICAL_SREG
                                                    | 0b10100000000000000000000000000000,
        RET                                         = DG0_BR_EG_SYS
                                                    | DG0_BR_EG_SYS_DG1_UBR_R
                                                    | 0b00000000010111110000000000000000,
        STP_PRE_IDX                                 = DG0_LS
                                                    | DG0_LS_DG1_LSRP_PRE_IDX
                                                    | 0b10000000000000000000000000000000,
        STR_POST_IDX                                = DG0_LS
                                                    | DG0_LS_DG1_LSR_POST_IDX
                                                    | 0b11000000000000000000000000000000,
        STR_PRE_IDX                                 = DG0_LS
                                                    | DG0_LS_DG1_LSR_PRE_IDX
                                                    | 0b11000000000000000000000000000000,
        STR_UNSIGNED_OFFSET                         = DG0_LS
                                                    | DG0_LS_DG1_LSR_UNSIGNED_IMM
                                                    | 0b11000000000000000000000000000000,
        SUBS_EREG                                   = DG0_DP_REG
                                                    | DG0_DP_REG_DG1_ADD_SUB_EREG
                                                    | 0b11100000000000000000000000000000,
    } arch_instr_t;

    typedef enum : uint8_t {
        // Extensions
        UXTX                                        = 0b00000011,
        SXTX                                        = 0b00000111,

        // Aliases
        LSL                                         = UXTX,
    } arch_ext_t;

    typedef enum : uint8_t {
        EQ                                          = 0b00000000,
        NE                                          = 0b00000001,
        GE                                          = 0b00001010,
        LT                                          = 0b00001011,
        GT                                          = 0b00001100,
        LE                                          = 0b00001101,
    } arch_cond_t;

    void jit_program();
    void jit_vm_instruction();
    void jit_deferred();

    void emit_vm_sub_entry_seq_from_host();
    void emit_vm_sub_exit_seq_to_host();
    void emit_non_vm_sub_entry_seq_to_host();
    void emit_non_vm_sub_exit_seq_to_host();

    void emit_sys_enter_stub();
    void emit_vm_reg_save_seq();
    void emit_reg_init();
    void emit_vm_exit_syscall_guard();

    arch_reg_t as_arch_reg(vm_reg_t reg) const      { return vr2ar.at(reg); }
    arch_reg_t as_arch_reg(uint8_t byte) const      { return as_arch_reg(static_cast<vm_reg_t>(byte)); }

    // Data processing
    void emit_add(arch_reg_t rd, arch_reg_t rs, uint16_t imm);
    void emit_adds_ereg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2);
    void emit_adr(arch_reg_t rd, int32_t imm);
    void emit_and_sreg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2);
    void emit_eor_sreg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2);
    void emit_movk(arch_reg_t rd, uint8_t shift, int16_t imm);
    void emit_movz(arch_reg_t rd, uint8_t shift, int16_t imm);
    void emit_orn_sreg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2);
    void emit_orr_sreg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2);
    void emit_subs_ereg(arch_reg_t rd, arch_reg_t rs1, arch_reg_t rs2);
                                            
    // Load/Store
    void emit_ldp_post_idx(arch_reg_t rd1, arch_reg_t rd2, arch_reg_t rb, int32_t imm);
    void emit_ldr_post_idx(arch_reg_t rd, arch_reg_t rb, int32_t imm);
    void emit_ldr_unsigned_offset(arch_reg_t rd, arch_reg_t rb, uint16_t imm);
    void emit_stp_pre_idx(arch_reg_t rs1, arch_reg_t rs2, arch_reg_t rb, int32_t imm);
    void emit_str_post_idx(arch_reg_t rs, arch_reg_t rb, int16_t imm);
    void emit_str_pre_idx(arch_reg_t rs, arch_reg_t rb, int16_t imm);
    void emit_str_unsigned_offset(arch_reg_t rs, arch_reg_t rb, uint16_t imm);

    // Branch
    void emit_b(int32_t imm);
    void emit_b_cond(arch_cond_t cond, int32_t imm);
    void emit_bl(int32_t imm);
    void emit_blr(arch_reg_t reg);
    void emit_br(arch_reg_t reg);
    void emit_nop();
    void emit_ret(arch_reg_t reg);

    // Other
    void emit_cmp_reg_imm(arch_reg_t rs, int64_t imm);
    void emit_cmp_reg_reg(arch_reg_t rs1, arch_reg_t rs2);
    void emit_mov_reg_imm(arch_reg_t rd, int64_t imm);
    void emit_mov_reg_reg(arch_reg_t rd, arch_reg_t rs);
    void emit_mov_reg_sp(arch_reg_t rd, arch_reg_t rs);
    void emit_pop_reg(arch_reg_t rd);
    void emit_push_reg(arch_reg_t rs);
    void emit_ret();

    void emit_sys_enter_call();
};
