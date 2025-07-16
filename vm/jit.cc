#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <pthread.h>
#include <regex>
#include <string>
#include <sys/mman.h>

#include "jit.h"


JIT::JIT(const void* prog, size_t prog_size, size_t mem_size_mb, bool debug)
: ExecutionEngine(prog, prog_size, mem_size_mb, debug)
, text_mem(nullptr), data_mem(nullptr)
, text_mem_size(mem_size / 4), data_mem_size(mem_size - text_mem_size)
, jpos({nullptr, (const uint8_t*) prog})
, sys_enter_stub(nullptr), stack(nullptr)
, reg_dump_area(new uint64_t[14])
{
}


void JIT::init_execution()
{
    init_memory();
    init_codegen();
}


void JIT::load_program()
{
    jit();
    flush_icache();
    if (debug)
        dump_code();
}


void JIT::exec_program()
{
    DBG("Running program ..." << endl);

    ((void (*)()) text_mem)();

    if (debug)
        dump_registers();
}


void JIT::fini_execution()
{
    fini_codegen();
    fini_memory();
}


void JIT::init_memory()
{
    DBG("Initializing memory ..." << endl);

    int prot, flags;

    prot = PROT_EXEC | PROT_READ | PROT_WRITE;
    flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef __APPLE__
    flags |= MAP_JIT;
#endif
    text_mem = (uint8_t*) mmap(nullptr, text_mem_size, prot, flags, 0, 0);
    if (text_mem == MAP_FAILED)
        ABORT("Failed to allocate text VM memory." << endl);

    prot = PROT_READ | PROT_WRITE;
    flags = MAP_PRIVATE | MAP_ANONYMOUS;
    data_mem = (uint8_t*) mmap(nullptr, data_mem_size, prot, flags, 0, 0);
    if (text_mem == MAP_FAILED)
        ABORT("Failed to allocate data VM memory." << endl);

    DBG("\t.text @" << (void*) text_mem << "[" << HEX(0, text_mem_size) << "]" << endl);
    DBG("\t.data @" << (void*) data_mem << "[" << HEX(0, data_mem_size) << "]" << endl);
}


void JIT::fini_memory()
{
    if (munmap(text_mem, text_mem_size) != 0)
        ABORT("Failed to deallocate text VM memory." << endl);
    text_mem = nullptr;
    if (munmap(data_mem, data_mem_size) != 0)
        ABORT("Failed to deallocate data VM memory." << endl);
    data_mem = nullptr;
}


void JIT::init_codegen()
{
    jpos.arch = text_mem;
    stack = data_mem + data_mem_size;
}


void JIT::fini_codegen()
{
}


void JIT::flush_icache()
{
    /* GCC/clang builtin, cross-platform: __builtin___clear_cache()
     *
     * Native alternatives:
     *      MacOS:  sys_icache_invalidate()
     *              https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/sys_icache_invalidate.3.html
     *      Linux:  cacheflush()
     *              https://man7.org/linux/man-pages/man2/cacheflush.2.html
     */
    __builtin___clear_cache((char*) text_mem, ((char*) text_mem) + text_mem_size);
}


void JIT::dump_code()
{
    DBG("JIT code dump:" << endl);

    {
        std::fstream bdf(BIN_DUMP_FILE, bdf.binary | bdf.out | bdf.trunc);
        if (!bdf.is_open())
            ABORT("Failed to open file '" << BIN_DUMP_FILE << "'." << endl);
        bdf.write((const char*) text_mem, text_mem_size);
    }

    {
        constexpr size_t cmd_len = 1024;
        static char cmd[cmd_len];
#ifdef __linux__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat"
#endif
        if (((size_t) std::snprintf(cmd, cmd_len, get_objdump_fmt(), (uint64_t) text_mem, BIN_DUMP_FILE, ASM_DUMP_FILE)) >= cmd_len)
#ifdef __linux__
    #pragma GCC diagnostic pop
#endif
            ABORT("Failed to prepare disassembly command." << endl);
        std::system(cmd);
    }

    {
        std::map<uint64_t, uint64_t> aa2va;
        for (const auto& e : va2aa)
            aa2va[e.second] = e.first;

        auto trace = [this, aa2va](std::string& line) {
            static std::regex whitespace_prefix("^\\s+");
            line = std::regex_replace(line, whitespace_prefix, "");

            auto get_vm_addr = [aa2va](const std::string& line) {
                static std::regex re_aa("^\\s*([0-9a-fA-F]+):.*$");
                std::smatch m_asm_aa;
                if (std::regex_match(line, m_asm_aa, re_aa)) {
                    auto m_jit_aa = aa2va.find(std::stoull(m_asm_aa[1].str(), nullptr, 16));
                    return m_jit_aa != aa2va.end() ? m_jit_aa->second : -1;
                }
                return (uint64_t) -1;
            };

            uint64_t va = get_vm_addr(line);
            if (va != (uint64_t) -1 && va != 0) {
                DBG(endl);
                trace_instr_decode(prog, va2idd.at(va));
                DBG(endl);
            }
        };

        std::fstream adf(ASM_DUMP_FILE, adf.in);
        if (!adf.is_open())
            ABORT("Failed to open file '" << ASM_DUMP_FILE << "'." << endl);
        std::string line;
        while (std::getline(adf, line)) {
            trace(line);
            DBG("\t" << line << endl);
        }
    }

    {
        if (std::remove(BIN_DUMP_FILE))
            ABORT("Failed to delete file '" << BIN_DUMP_FILE << "'." << endl);
        if (std::remove(ASM_DUMP_FILE))
            ABORT("Failed to delete file '" << ASM_DUMP_FILE << "'." << endl);
    }
}


void JIT::dump_registers()
{
    DBG("Registers:" << endl);
    DBG("\tR0    = " << HEX(16, reg_dump_area[0])   << endl);
    DBG("\tR1    = " << HEX(16, reg_dump_area[1])   << endl);
    DBG("\tR2    = " << HEX(16, reg_dump_area[2])   << endl);
    DBG("\tR3    = " << HEX(16, reg_dump_area[3])   << endl);
    DBG("\tR4    = " << HEX(16, reg_dump_area[4])   << endl);
    DBG("\tR5    = " << HEX(16, reg_dump_area[5])   << endl);
    DBG("\tR6    = " << HEX(16, reg_dump_area[6])   << endl);
    DBG("\tR7    = " << HEX(16, reg_dump_area[7])   << endl);
    DBG("\tR8    = " << HEX(16, reg_dump_area[8])   << endl);
    DBG("\tR9    = " << HEX(16, reg_dump_area[9])   << endl);
    DBG("\tR10   = " << HEX(16, reg_dump_area[10])  << endl);
    DBG("\tR11   = " << HEX(16, reg_dump_area[11])  << endl);
    DBG("\tR12   = " << HEX(16, reg_dump_area[12])  << endl);
    DBG("\tFLAGS = " << "N/A"                       << endl);
    DBG("\tSP    = " << HEX(16, reg_dump_area[13])  << endl);
    DBG("\tPC    = " << "N/A"                       << endl);
}



void JIT::record_addr_mapping()
{
    va2aa[(uint64_t) jpos.vm - (uint64_t) prog] = (uint64_t) jpos.arch;
}


uint64_t JIT::as_arch_addr(uint64_t vm_addr) const
{
    auto m = va2aa.find(vm_addr);
    return m != va2aa.end() ? m->second : -1;
}


uint64_t* JIT::sys_enter(uint64_t* sp)
{
    uint64_t syscall_id = *(sp + 1);

    switch (syscall_id) {
    case SYSCALL_VM_EXIT:
        ABORT("Internal error. SYSCALL_VM_EXIT should not have been handled here." << endl);
    case SYSCALL_DISPLAY_SINT: {
        int64_t val = *(sp + 2);
        cout << val << endl;
        *(sp + 2) = *(sp + 0);
        sp += 2;
        break;
    }
    case SYSCALL_DISPLAY_UINT: {
        uint64_t val = *(sp + 2);
        cout << val << endl;
        *(sp + 2) = *(sp + 0);
        sp += 2;
        break;
    }
    default:
        ABORT("Unsupported syscall ID '" << syscall_id << "'." << endl);
    }

    return sp;
}
