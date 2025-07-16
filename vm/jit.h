#pragma once


#include <map>
#include <vector>

#include "exe.h"


class JIT : public ExecutionEngine {
public:
    JIT(const void* prog, size_t prog_size, size_t mem_size_mb, bool debug);

protected:
    static constexpr const char* BIN_DUMP_FILE      = "jit.bin";
    static constexpr const char* ASM_DUMP_FILE      = "jit.s";

    uint8_t                                         *text_mem;
    uint8_t                                         *data_mem;
    size_t                                          text_mem_size, data_mem_size;

    typedef struct {
        uint8_t                                     *arch;
        const uint8_t                               *vm;
    } jit_pos_t;

    jit_pos_t                                       jpos;
    std::vector<jit_pos_t>                          deferred_jpos;

    uint8_t                                         *sys_enter_stub;
    uint8_t                                         *stack;
    std::map<uint64_t, uint64_t>                    va2aa;
    std::map<uint64_t, instr_decode_data_t>         va2idd;

    std::unique_ptr<uint64_t[]>                     reg_dump_area;

    void init_execution() override;
    void load_program() override;
    void exec_program() override;
    void fini_execution() override;

    virtual void jit() = 0;

    void init_memory();
    void fini_memory();
    void init_codegen();
    void fini_codegen();
    void flush_icache();

    virtual const char* get_objdump_fmt() const = 0;
    void dump_code();
    void dump_registers();

    void record_addr_mapping();
    uint64_t as_arch_addr(uint64_t vm_addr) const;
 
    static uint64_t* sys_enter(uint64_t* sp);
};
