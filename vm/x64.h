#pragma once


#include <map>

#include "jit.h"


class x86_64JIT final : public JIT {
public:
    x86_64JIT(const void* prog, size_t prog_size, size_t mem_size_mb, bool debug);

private:
    static constexpr const char* OBJDUMP_FMT        = "objdump -b binary -m i386:x86-64 -M intel --adjust-vma 0x%llx -D %s > %s";
    const char* get_objdump_fmt() const override    { return OBJDUMP_FMT; }

    uint64_t                                        host_sp;
    uint64_t                                        vm_sp;

    void jit() override;

    typedef enum : uint8_t {
        RAX                                         = 0b00000000,
        RCX                                         = 0b00000001,
        RDX                                         = 0b00000010,
        RBX                                         = 0b00000011,
        RSP                                         = 0b00000100,
        RBP                                         = 0b00000101,
        RSI                                         = 0b00000110,
        RDI                                         = 0b00000111,
        R8                                          = 0b00001000,
        R9                                          = 0b00001001,
        R10                                         = 0b00001010,
        R11                                         = 0b00001011,
        R12                                         = 0b00001100,
        R13                                         = 0b00001101,
        R14                                         = 0b00001110,
        R15                                         = 0b00001111,
    } arch_reg_t;
    static constexpr uint8_t ARCH_REG_MASK          = 0b00000111;

    static const std::map<vm_reg_t, arch_reg_t> vr2ar;

    typedef enum : uint8_t {
        REX_W                                       = 0b01001000,
        REX_R                                       = 0b01000100,
        REX_X                                       = 0b01000010,
        REX_B                                       = 0b01000001,
    } arch_rex_prefix_t;

    typedef enum : uint8_t {
        MOD_B0D                                     = 0b00000000,
        MOD_B8D                                     = 0b01000000,
        MOD_B32D                                    = 0b10000000,
        MOD_R                                       = 0b11000000,
    } arch_mod_t;

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

    static arch_reg_t reg_base(arch_reg_t r)        { return static_cast<arch_reg_t>(r & ARCH_REG_MASK); }
    static arch_rex_prefix_t rex_adj_r(arch_reg_t r)
                                                    {
                                                        return static_cast<arch_rex_prefix_t>(
                                                            (reg_base(r) != r) ? REX_R : 0
                                                        );
                                                    }
    static arch_rex_prefix_t rex_adj_m(arch_reg_t m)
                                                    {
                                                        return static_cast<arch_rex_prefix_t>(
                                                            (reg_base(m) != m) ? REX_B : 0
                                                        );
                                                    }
    static arch_rex_prefix_t rex_adj_rm(arch_reg_t r, arch_reg_t m)
                                                    {
                                                        return static_cast<arch_rex_prefix_t>(
                                                            rex_adj_r(r) | rex_adj_m(m)
                                                        );
                                                    }

    static constexpr uint8_t ADD_R_R[]              = { REX_W, 0x03, 0x00                                           };
    static constexpr uint8_t AND_R_R[]              = { REX_W, 0x23, 0x00                                           };
    static constexpr uint8_t CALL_R[]               = { REX_W, 0xff, 0x00                                           };
    static constexpr uint8_t CMP_R_R[]              = { REX_W, 0x39, 0x00                                           };
    static constexpr uint8_t JE_IMM32[]             = { 0x0f,  0x84, 0x00, 0x00, 0x00, 0x00                         };
    static constexpr uint8_t JNE_IMM32[]            = { 0x0f,  0x85, 0x00, 0x00, 0x00, 0x00                         };
    static constexpr uint8_t JG_IMM32[]             = { 0x0f,  0x8f, 0x00, 0x00, 0x00, 0x00                         };
    static constexpr uint8_t JGE_IMM32[]            = { 0x0f,  0x8d, 0x00, 0x00, 0x00, 0x00                         };
    static constexpr uint8_t JL_IMM32[]             = { 0x0f,  0x8c, 0x00, 0x00, 0x00, 0x00                         };
    static constexpr uint8_t JLE_IMM32[]            = { 0x0f,  0x8e, 0x00, 0x00, 0x00, 0x00                         };
    static constexpr uint8_t JMP_IMM32[]            = { 0xe9,  0x00, 0x00, 0x00, 0x00                               };
    static constexpr uint8_t JMP_R[]                = { REX_W, 0xff, 0x00                                           };
    static constexpr uint8_t MOV_BD_R[]             = { REX_W, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00             };
    static constexpr uint8_t MOV_R_BD[]             = { REX_W, 0x8b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00             };
    static constexpr uint8_t MOV_R_IMM32[]          = { REX_W, 0xc7, 0x00, 0x00, 0x00, 0x00, 0x00                   };
    static constexpr uint8_t MOV_R_IMM64[]          = { REX_W, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static constexpr uint8_t MOV_R_R[]              = { REX_W, 0x8b, 0x00                                           };
    static constexpr uint8_t NOP[]                  = { 0x90                                                        };
    static constexpr uint8_t NOT_R[]                = { REX_W, 0xf7, 0x00                                           };
    static constexpr uint8_t OR_R_R[]               = { REX_W, 0x0b, 0x00                                           };
    static constexpr uint8_t POP_REG[]              = { REX_B, 0x58                                                 };
    static constexpr uint8_t PUSH_REG[]             = { REX_B, 0x50                                                 };
    static constexpr uint8_t RET[]                  = { 0xc3                                                        };
    static constexpr uint8_t SUB_R_R[]              = { REX_W, 0x2b, 0x00                                           };
    static constexpr uint8_t XOR_R_R[]              = { REX_W, 0x33, 0x00                                           };

    // Data processing
    void emit_add_reg_imm64(arch_reg_t rd, int64_t imm);
    void emit_add_reg_reg(arch_reg_t rd, arch_reg_t rs);
    void emit_and_reg_imm64(arch_reg_t rd, int64_t imm);
    void emit_and_reg_reg(arch_reg_t rd, arch_reg_t rs);
    void emit_cmp_reg_imm64(arch_reg_t rs, int64_t imm);
    void emit_cmp_reg_reg(arch_reg_t rs1, arch_reg_t rs2);
    void emit_or_reg_imm64(arch_reg_t rd, int64_t imm);
    void emit_or_reg_reg(arch_reg_t rd, arch_reg_t rs);
    void emit_not_reg(arch_reg_t r);
    void emit_sub_reg_imm64(arch_reg_t rd, int64_t imm);
    void emit_sub_reg_reg(arch_reg_t rd, arch_reg_t rs);
    void emit_xor_reg_imm64(arch_reg_t rd, int64_t imm);
    void emit_xor_reg_reg(arch_reg_t rd, arch_reg_t rs);

    // Load/Store
    void emit_mov_b8d_reg(arch_reg_t rb, int8_t d, arch_reg_t rs);
    void emit_mov_reg_b8d(arch_reg_t rd, arch_reg_t rb, int8_t d);
    void emit_mov_b32d_reg(arch_reg_t rb, int32_t d, arch_reg_t rs);
    void emit_mov_reg_b32d(arch_reg_t rd, arch_reg_t rb, int32_t d);
    void emit_mov_reg_imm(arch_reg_t rd, int64_t imm);
    void emit_mov_reg_imm32(arch_reg_t rd, int32_t imm);
    void emit_mov_reg_imm64(arch_reg_t rd, int64_t imm);
    void emit_mov_reg_reg(arch_reg_t rd, arch_reg_t rs);

    // Branch
    void emit_call_imm64(uint64_t imm);
    void emit_call_reg(arch_reg_t rs);
    void emit_je_imm32(int32_t imm);
    void emit_jne_imm32(int32_t imm);
    void emit_jg_imm32(int32_t imm);
    void emit_jge_imm32(int32_t imm);
    void emit_jl_imm32(int32_t imm);
    void emit_jle_imm32(int32_t imm);
    void emit_jmp_imm32(int32_t imm);
    void emit_jmp_imm64(uint64_t imm);
    void emit_jmp_reg(arch_reg_t rs);
    void emit_nop();

    // Other
    void emit_pop_reg(arch_reg_t rd);
    void emit_push_reg(arch_reg_t rs);
    void emit_ret();

    void emit_sys_enter_call();
};
