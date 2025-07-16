#include <cstddef>
#include <memory>

#include "vm.h"
#include "exe.h"
#include "int.h"
#include "a64.h"
#include "x64.h"


static size_t adjust_mem_size_mb(size_t mem_size_mb);
static ExecutionEngine* create_execution_engine(
    const void* prog, size_t prog_size, size_t mem_size_mb, exec_type_t exec_type, bool debug);


extern "C"
void vm_run(
    const void* prog,
    size_t prog_size,
    size_t mem_size_mb,
    exec_type_t exec_type,
    bool debug
)
{
    std::unique_ptr<ExecutionEngine>(
        create_execution_engine(
            prog,
            prog_size,
            adjust_mem_size_mb(mem_size_mb),
            exec_type,
            debug
    ))->execute();
}


static size_t adjust_mem_size_mb(size_t mem_size_mb)
{
    size_t size = 0x4;
    while (mem_size_mb > size)
        size <<= 1;
    return size;
}


static ExecutionEngine* create_execution_engine(
    const void* prog, size_t prog_size, size_t mem_size_mb, exec_type_t exec_type, bool debug)
{
    switch (exec_type) {
    case INTERPRETER:
        return new Interpreter(prog, prog_size, mem_size_mb, debug);
    case AArch64JIT:
        return new class AArch64JIT(prog, prog_size, mem_size_mb, debug);
    case x86_64JIT:
        return new class x86_64JIT(prog, prog_size, mem_size_mb, debug);
    default:
        ABORT("Unsupported execution type ID '" << exec_type << "'." << endl);
    }
}
