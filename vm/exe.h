#pragma once


#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>


using std::cout, std:: endl;


#define DBG_(DATA) { if (debug) { cout << DATA; } }
#define DBG(DATA) DBG_("[DEBUG] " << DATA)

#define ERR_(DATA) { cout << DATA; }
#define ERR(DATA) ERR_("[ERROR] " << DATA)

#define ABORT(DATA) { \
    ERR(DATA); \
    std::abort(); \
}

#define HEX_(WIDTH, DATA) \
    std::setw(WIDTH) << \
    std::setfill('0') << std::right << std::hex << std::noshowbase << DATA << \
    std::dec
#define HEX(WIDTH, DATA) "0x" << HEX_(WIDTH, DATA)
#define HEX_0(DATA) "0x" << HEX_(0, DATA)


class ExecutionEngine {
protected:
    const void* prog;
    size_t prog_size;
    size_t mem_size;
    bool debug;

    virtual void init_execution() = 0;
    virtual void load_program() = 0;
    virtual void exec_program() = 0;
    virtual void fini_execution() = 0;

public:
    ExecutionEngine(const void* prog, size_t prog_size, size_t mem_size_mb, bool debug);
    virtual ~ExecutionEngine();

    virtual void execute() final {
        init_execution();
        load_program();
        exec_program();
        fini_execution();
    }

protected:
    typedef enum : uint8_t {
        LOAD        =  1,
        STORE       =  2,
        MOV         =  3,
        ADD         =  4,
        SUB         =  5,
        AND         =  6,
        OR          =  7,
        XOR         =  8,
        NOT         =  9,
        CMP         = 10,
        PUSH        = 11,
        POP         = 12,
        CALL        = 13,
        RET         = 14,
        JMP         = 15,
        JMPEQ       = 16,
        JMPNE       = 17,
        JMPGT       = 18,
        JMPLT       = 19,
        JMPGE       = 20,
        JMPLE       = 21,
    } vm_instr_t;
    
    typedef enum : uint8_t {
        R0          =  0,
        R1          =  1,
        R2          =  2,
        R3          =  3,
        R4          =  4,
        R5          =  5,
        R6          =  6,
        R7          =  7,
        R8          =  8,
        R9          =  9,
        R10         = 10,
        R11         = 11,
        R12         = 12,
        FLAGS       = 13,
        SP          = 14,
        PC          = 15,
    } vm_reg_t;

    typedef enum : uint8_t {
        REG         =  0,
        IMM         =  1,
    } vm_am_t;

    static const uint64_t SYS_ENTER_ADDR            = 0x0;
    static const uint64_t SYSCALL_VM_EXIT           = 0;
    static const uint64_t SYSCALL_DISPLAY_SINT      = 1;
    static const uint64_t SYSCALL_DISPLAY_UINT      = 2;

    static const uint64_t FLAG_EQ                   = 0b00000001;
    static const uint64_t FLAG_LT                   = 0b00000010;
    static const uint64_t FLAG_GT                   = 0x00000100;

    static const uint8_t INSTRUCTION_MASK           = 0xfc;
    static const uint8_t ACCESS_MODE_MASK           = 0x01;
    static const uint8_t REG_DST_MASK               = 0xf0;
    static const uint8_t REG_SRC_MASK               = 0x0f;

    static uint8_t   instr(uint8_t byte)            { return (byte & INSTRUCTION_MASK) >> 2; }
    static uint8_t   access_mode(uint8_t byte)      { return byte & ACCESS_MODE_MASK; }
    static uint8_t   reg_dst(uint8_t byte)          { return (byte & REG_DST_MASK) >> 4; }
    static uint8_t   reg_src(uint8_t byte)          { return byte & REG_SRC_MASK; }
    static uint64_t  imm64u(const uint8_t& data)    { return as_dword(const_cast<uint8_t&>(data)); }
    static int64_t   imm64s(const uint8_t& data)    { return as_signed(as_dword(const_cast<uint8_t&>(data))); }
    static int16_t   imm16s(const uint8_t& data)    { return as_signed(as_hword(const_cast<uint8_t&>(data))); }

    static uint16_t& as_hword(uint8_t& val)         { return reinterpret_cast<uint16_t&>(val); }
    static uint64_t& as_dword(uint8_t& val)         { return reinterpret_cast<uint64_t&>(val); }
    static int64_t&  as_signed(uint64_t& val)       { return reinterpret_cast<int64_t&>(val); }
    static int16_t&  as_signed(uint16_t& val)       { return reinterpret_cast<int16_t&>(val); }

    typedef struct {
        uint64_t    addr;
        uint8_t     am;
        uint8_t     dst;
        uint8_t     src;
        int16_t     idx;
        uint64_t    ivu;
        int64_t     ivs;
    } instr_decode_data_t;

    void trace_instr_decode(const void* mem, const instr_decode_data_t& idd) const;
};
