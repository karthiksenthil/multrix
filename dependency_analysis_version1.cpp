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
int total_main_ins=0;
int **ndm;
/* memoryWriteMap */
/* key => memory address written */
/* value => vector of writing instructions */ /* ( sorted by default ? ) */
std::map < VOID*, vector<int> > memoryWriteMap;  

/* dependencyMap */
/* key => instruction */
/* value => instruction that key depends on */ /* doubt --> is it one instruction or array of ins ? */
std::map < int, int > dependencyMap;

FILE *fp;

INT32 Usage()
{
    cerr << "This tool identifies the main function and gets" << endl;
    cerr << " data dependencies in them." << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

VOID RecordMemRead(VOID *ip, VOID *addr, VOID *rbp, int ins_count)
{
    
    if(memoryWriteMap.count(addr) > 0) // check if key exists
    {
        dependencyMap[ins_count] = memoryWriteMap[addr].back();
    }

    /* Code to convert hex to decimal and find offset from rbp*/
    INT64 x;
    std::stringstream ss;
    ss << std::hex << addr;
    ss >> x;
    fprintf(fp, "%d %ld\r\n", memory_access_count++, rbp_int64-x);

}

VOID RecordMemWrite(VOID *ip, VOID *addr, int ins_count)
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
    memoryWriteMap[addr].push_back(ins_count);

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
    ++total_main_ins;
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
                IARG_UINT32, (UINT32)total_main_ins,
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
                IARG_UINT32, (UINT32)total_main_ins,
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
        }

        RTN_Close(rtn);
    }
}
 
VOID Fini(INT32 code, VOID *v)
{
    std::cout << "Inside final func." << std::endl;
    
    /* code to check contents of dependencyMap */
    /* Construction on Node Dependency Matrix */

    ndm = (int **)malloc(total_main_ins * sizeof(int *));
    for (int i = 0; i < total_main_ins; i++)
        ndm[i] = (int *)malloc(total_main_ins * sizeof(int));

    for(std::map< int,int >::iterator iter=dependencyMap.begin();iter!=dependencyMap.end();iter++)
    {
        ndm[iter->first-1][iter->second-1] = 1;
        std::cout << iter->first <<" depends on " << iter->second << std::endl;
    }

    std::cout << "Total main instructions: " << total_main_ins << std::endl;

    std::cout << "\n\n ************** Final NDM ************** \n\n  ";

    for(int i=0; i < total_main_ins; i++)
        std::cout << i+1 << " ";
    std::cout << endl;
    for(int i=0; i < total_main_ins; i++)
    {
        std::cout << i+1 << " ";
        for(int j=0; j < total_main_ins; j++)
        {
            std::cout << ndm[i][j] << " ";
        }
        std::cout << std::endl;
    }



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