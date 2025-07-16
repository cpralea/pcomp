#pragma once


#include <cstddef>
#include <cstdint>


typedef enum : uint8_t {
    INTERPRETER = 1,
    AArch64JIT  = 2,
    x86_64JIT   = 3
} exec_type_t;


extern "C"
void vm_run(
    const void* prog,
    size_t prog_size,
    size_t mem_size_mb,
    exec_type_t exec_type,
    bool debug
);
