#pragma once


#include "exe.h"


class Interpreter final : public ExecutionEngine {
public:
    Interpreter(const void* prog, size_t prog_size, size_t mem_size_mb, bool debug);

private:
    uint8_t* mem;
    uint64_t reg[16];

    void init_execution() override;
    void load_program() override;
    void exec_program() override;
    void fini_execution() override;

    void sys_enter();

    void dump_registers() const;
};
