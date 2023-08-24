/*
 * Copyright (C) 2004-2021 Intel Corporation.
 * SPDX-License-Identifier: MIT
 */

/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include <stdio.h>
#include "pin.H"
#include <iostream>

// FILE* trace;
// Do this for all instructions.
VOID RecordIP(VOID *ip) {
    fprintf(stderr, "PC:%p\n", ip); 
}

// Print a memory read record
VOID RecordMemRead(VOID* ip, VOID* addr) {
    fprintf(stderr, "%p: R %p\n", ip, addr); 
}

// Print a memory write record
VOID RecordMemWrite(VOID* ip, VOID* addr) { fprintf(stderr, "%p: W %p\n", ip, addr); }

VOID RecordRegWrite(VOID *ip, unsigned long long reg_value, unsigned long reg_number) { 
    
    struct timespec curr;
    /*if(reg_value == NULL) {
        fprintf(stdout, "Reg is null\n");
        return;
    } */
    if(reg_value < 4096) 
        return; 

    clock_gettime(CLOCK_MONOTONIC, &curr);
    fprintf(stderr, "%ld.%ld IP:%p Reg:%llu RegNo:%lu\n", curr.tv_sec, curr.tv_nsec, ip,reg_value, reg_number); 
}
// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID* v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP
    // prefixed instructions appear as predicated instructions in Pin.
    // INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) RecordIP, IARG_INST_PTR,IARG_END);
    UINT32 memOperands = INS_OperandCount(ins);
    // UINT32 mem_values = INS_MemoryOperandCount(ins);
    bool is_reg_written = false; 
    // Instrument all Loads and LEA
    if(INS_IsMemoryRead(ins)) {
        // Check all the operands of the instruction.
        for (UINT32 memOp = 0; memOp < memOperands; memOp++)
        {

               REG reg = INS_OperandReg(ins, memOp);

                if(reg == REG_INVALID()) {
                    continue; 
                }
                
                REG regname = REG_FullRegName(reg);
                
                if (regname == REG_INVALID()) { 
                    continue;
                }

                
                if (INS_RegWContain(ins, reg)) {
                    // is_reg_written = true;
                    if(INS_IsValidForIpointAfter(ins) && REG_valid_for_iarg_reg_value(reg)) {
                        INS_InsertPredicatedCall(ins, IPOINT_AFTER, (AFUNPTR)RecordRegWrite, IARG_INST_PTR, IARG_REG_VALUE, reg, IARG_ADDRINT, regname, IARG_END);
                        is_reg_written = true;
                    }
                }
                
        }
    
    }

    memOperands = INS_MemoryOperandCount(ins);
 
    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands && is_reg_written; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(ins, IPOINT_AFTER, (AFUNPTR)RecordMemRead, IARG_INST_PTR, IARG_MEMORYOP_EA, memOp,
                                     IARG_END);
        }
    }

}

VOID Fini(INT32 code, VOID* v)
{
    fprintf(stderr, "#eof\n");
    fclose(stderr);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints a trace of memory addresses\n" + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char* argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    // trace = fopen("pinatrace.out", "w");
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
