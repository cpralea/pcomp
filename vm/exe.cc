#include "exe.h"


ExecutionEngine::ExecutionEngine(const void* prog, size_t prog_size, size_t mem_size_mb, bool debug)
: prog(prog), prog_size(prog_size), mem_size(mem_size_mb << 20), debug(debug)
{
    DBG("Initializing VM with:" << endl);
    DBG("\tprogram at " << prog << ", size " << prog_size << endl);
    DBG("\tmemory " << mem_size_mb << " MiB" << endl);
}


ExecutionEngine::~ExecutionEngine()
{
}


void ExecutionEngine::trace_instr_decode(const void* mem, const instr_decode_data_t& idd) const
{
    if (!debug)
        return;

    static const char* const R[] = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "flags", "sp", "pc"
    };

    const uint8_t* addr = (const uint8_t*) mem + idd.addr;

    auto HEX_DUMP = [this, addr](uint8_t num_bytes) {
        for (uint8_t i = 0; i < num_bytes; i++)
            DBG_(HEX_(2, ((int) *(addr + i))) << " ");
        for (uint8_t i = 0; i < 10 - num_bytes; i++)
            DBG_("   ");
        DBG_("   ");
    };
    auto IDX_DUMP = [this](int32_t idx) {
        if (idx == 0)
            return;
        if (idx > 0) DBG_(" + " << idx) else DBG_(" - " << -idx);
    };

    uint8_t i = instr(*addr);

    DBG("vm >\t" << HEX(5, idd.addr) << "   ");
    switch (i) {
    case LOAD:
        HEX_DUMP(4);           DBG_("load " << R[idd.dst] << ", [" << R[idd.src]); IDX_DUMP(idd.idx); DBG_("]");    break;
    case STORE:
        HEX_DUMP(4);           DBG_("store [" << R[idd.dst]); IDX_DUMP(idd.idx); DBG_("], " << R[idd.src]);         break;
    case MOV:
        HEX_DUMP(idd.am?10:2); DBG_("mov " << R[idd.dst] << ", "); if (idd.am) DBG_(idd.ivs) else DBG_(R[idd.src]); break;
    case ADD:
        HEX_DUMP(idd.am?10:2); DBG_("add " << R[idd.dst] << ", "); if (idd.am) DBG_(idd.ivs) else DBG_(R[idd.src]); break;
    case SUB:
        HEX_DUMP(idd.am?10:2); DBG_("sub " << R[idd.dst] << ", "); if (idd.am) DBG_(idd.ivs) else DBG_(R[idd.src]); break;
    case AND:
        HEX_DUMP(idd.am?10:2); DBG_("and " << R[idd.dst] << ", "); if (idd.am) DBG_(idd.ivu) else DBG_(R[idd.src]); break;
    case OR:
        HEX_DUMP(idd.am?10:2); DBG_("or " << R[idd.dst] << ", "); if (idd.am) DBG_(idd.ivu) else DBG_(R[idd.src]);  break;
    case XOR:
        HEX_DUMP(idd.am?10:2); DBG_("xor " << R[idd.dst] << ", "); if (idd.am) DBG_(idd.ivu) else DBG_(R[idd.src]); break;
    case NOT:
        HEX_DUMP(2);           DBG_("not " << R[idd.dst]);                                                          break;
    case CMP:
        HEX_DUMP(idd.am?10:2); DBG_("cmp " << R[idd.dst] << ", "); if (idd.am) DBG_(idd.ivs) else DBG_(R[idd.src]); break;
    case PUSH:
        HEX_DUMP(2);           DBG_("push " << R[idd.dst]);                                                         break;
    case POP:
        HEX_DUMP(2);           DBG_("pop " << R[idd.dst]);                                                          break;
    case CALL:
        HEX_DUMP(9);           DBG_("call " << HEX_0(idd.ivu));                                                     break;
    case RET:
        HEX_DUMP(1);           DBG_("ret");                                                                         break;
    case JMP:
        HEX_DUMP(9);           DBG_("jmp " << HEX_0(idd.ivu));                                                      break;
    case JMPEQ:
        HEX_DUMP(9);           DBG_("jmpeq " << HEX_0(idd.ivu));                                                    break;
    case JMPNE:
        HEX_DUMP(9);           DBG_("jmpne " << HEX_0(idd.ivu));                                                    break;
    case JMPGT:
        HEX_DUMP(9);           DBG_("jmpgt " << HEX_0(idd.ivu));                                                    break;
    case JMPLT:
        HEX_DUMP(9);           DBG_("jmplt " << HEX_0(idd.ivu));                                                    break;
    case JMPGE:
        HEX_DUMP(9);           DBG_("jmpge " << HEX_0(idd.ivu));                                                    break;
    case JMPLE:
        HEX_DUMP(9);           DBG_("jmple " << HEX_0(idd.ivu));                                                    break;
    default:
        ABORT("Unsupported instruction  '" << HEX(2, i) << "'." << endl);
    }
    DBG_(endl);
}
