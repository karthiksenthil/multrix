#include "pin.H"
#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <sstream>
#include <map>
#include <vector>
using namespace std;
 

/* global counter for memory access pattern */
int memory_access_count = 0;
bool first_memory_access = false;
VOID* first_written_rbp;
INT64 rbp_int64;
INT64 total_main_ins=0;
/* memoryWriteMap */
/* key => memory address written */
/* value => vector of writing instructions */ /* ( sorted by default ? ) */
std::map < VOID*, vector<VOID*> > memoryWriteMap;  

/* dependencyMap */
/* key => instruction */
/* value => instruction that key depends on */ /* doubt --> is it one instruction or array of ins ? */
std::map < VOID*, VOID* > dependencyMap;

FILE *fp;

INT32 Usage()
{
    cerr << "This tool identifies the main function and gets" << endl;
    cerr << " data dependencies in them." << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

VOID RecordMemRead(VOID *ip, VOID *addr, VOID *rbp)
{
    std::cout << "Instruction " << ip << " reads from memory " << addr << std::endl;
    /*for(UINT32 write_ins = 0; write_ins < memoryWriteMap[addr].size(); write_ins++ )
    {
        if(ip > memoryWriteMap[addr][write_ins])
        {
            // std::cout << ip << " > " << memoryWriteMap[addr][write_ins] << std::endl;

        }
    }*/

    if(memoryWriteMap.count(addr) > 0) // check if key exists
    {
        dependencyMap[ip] = memoryWriteMap[addr].back();
    }

    /* Code to convert hex to decimal and find offset from rbp*/
    INT64 x;
    std::stringstream ss;
    ss << std::hex << addr;
    ss >> x;
    std::cout << x << std::endl;
    fprintf(fp, "%d %ld\r\n", memory_access_count++, rbp_int64-x);

}

VOID RecordMemWrite(VOID *ip, VOID *addr)
{    
    /* Record the first write, make the flag true and make it base pointer value*/
    if(!first_memory_access)
    {
        std::stringstream ss1;
        first_written_rbp = addr;
        ss1 << std::hex << first_written_rbp;
        ss1 >> rbp_int64;
        first_memory_access = true;
    }
    std::cout << "Instruction " << ip << " writes to memory " << addr << std::endl;
    memoryWriteMap[addr].push_back(ip);

    /* Code to convert hex to decimal and find offset from rbp */
    INT64 x;
    std::stringstream ss;
    ss << std::hex << addr;
    ss >> x;
    fprintf(fp, "%d %ld\r\n", memory_access_count++, rbp_int64-x);
}


VOID MainInstruction(INS ins)
{
    std::cout << INS_Disassemble(ins) << std::endl;
    
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    for(UINT32 memOp = 0; memOp < memOperands; memOp++)
    {

        /* Memory Read Instruction */
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            // std::cout << INS_Disassemble(ins) << std::endl;
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_REG_VALUE,REG_SEG_FS_BASE,
                IARG_END);
        }

        /* Memory Write Instruction */
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            // std::cout << INS_Disassemble(ins) << std::endl;
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
    }
}


VOID Routine(RTN rtn, VOID *v)
{
 
    //std::cout << "Helloo" << std::endl;
    //RTN rtn = RTN_FindByName( img, "main" );
    
    if (RTN_Valid(rtn) && RTN_Name(rtn)=="main")
    {        
        //std::cout << RTN_Name(rtn) << std::endl;
        //std::cout << RTN_Address(rtn) << std::endl;

        RTN_Open(rtn);

        for(INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
        {
            //INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)MainInstruction, ins, IARG_END);
            MainInstruction(ins);
            ++total_main_ins;
        }

        RTN_Close(rtn);
    }
}
 
VOID Fini(INT32 code, VOID *v)
{
    std::cout << "I am Fini" << std::endl;

    std::cout << "First Written RBP: " << first_written_rbp << std::endl;
    std::cout << rbp_int64 << std::endl;
    /* code to check contents of memoryWriteMap */
    for(std::map< VOID*,vector<VOID*> >::iterator iter=memoryWriteMap.begin();iter!=memoryWriteMap.end();iter++)
    {
        std::cout << "Memory Address : " << iter->first << std::endl;
        std::cout << "Writing Instructions" << std::endl;
        /* is sorting of vector of instructions needed here ? */
        for(UINT32 i=0; i < (iter->second).size(); i++)
        {
            std::cout << (iter->second)[i] << std::endl;
        }
    }

    /* code to check contents of dependencyMap */
    for(std::map< VOID*,VOID* >::iterator iter=dependencyMap.begin();iter!=dependencyMap.end();iter++)
    {
        std::cout << iter->first <<" depends on " << iter->second << std::endl;
    }

    std::cout << "Total main instructions: " << total_main_ins << std::endl;
}
 
 
int main( int argc, char *argv[] )
{   
    PIN_InitSymbols();

    fp = fopen("mem_access.dat","w");
 
    if (PIN_Init(argc, argv)) return Usage();
 
    RTN_AddInstrumentFunction(Routine, 0 );
    PIN_AddFiniFunction(Fini, 0);
 
    PIN_StartProgram();
 
    return 0;
}