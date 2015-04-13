#include "pin.H"
#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;
 
/* Function prototypes */
VOID ProcedureCall(UINT32 ip);


/* global counter for memory access pattern */
int memory_access_count = 0;
bool first_memory_access = false;
VOID* first_written_rbp;
INT64 rbp_int64;
int total_main_ins=0;

/* 2-D Boolean array of instructions vs. instructions */
int **ndm;

/* memoryWriteMap */
/* key => memory address written */
/* value => vector of writing instructions */ /* ( sorted by default ? ) */
std::map < VOID*, vector<int> > memoryWriteMap;  

/* dependencyMap */
/* key => instruction */
/* value => instructions that key depends on */ /* doubt --> is it one instruction or array of ins ? */
std::map < int, vector<int> > dependencyMap;

FILE *fp;

INT32 Usage()
{
    cerr << "This tool identifies the functions of a program and gets" << endl;
    cerr << " data dependencies in them." << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

VOID RecordMemRead(VOID *ip, VOID *addr, VOID *rbp, int ins_count)
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
        // dependencyMap[ins_count] = memoryWriteMap[addr].back();

        /* Add only unique instructions to the dependency vector for an ins */
        /* remove the if block to demo loop identification idea */
        if(std::find(dependencyMap[ins_count].begin(), dependencyMap[ins_count].end(), memoryWriteMap[addr].back()) == dependencyMap[ins_count].end())
        {
            dependencyMap[ins_count].push_back(memoryWriteMap[addr].back());
        }
    }

    /* Code to convert hex to decimal and find offset from rbp*/
    INT64 x;
    std::stringstream ss;
    ss << std::hex << addr;
    ss >> x;
    std::cout << x << std::endl;
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
    
    /* Code for adding WAW dependency i.e. if two instructions are writing to */
    /* same address the second depends on first for sequential logic */
    if(memoryWriteMap.count(addr) > 0)
    {
        std::cout << "Instruction " << ins_count << " depends on " << memoryWriteMap[addr].back() << std::endl;
        // dependencyMap[ins_count] = memoryWriteMap[addr].back();

        /* Add only unique instructions to the dependency vector for an ins */
        /* remove the if block to demo loop identification idea */
        if(std::find(dependencyMap[ins_count].begin(), dependencyMap[ins_count].end(), memoryWriteMap[addr].back()) == dependencyMap[ins_count].end())
        {
            dependencyMap[ins_count].push_back(memoryWriteMap[addr].back());
        }
    }

    std::cout << "Instruction " << ip << " writes to memory " << addr << std::endl;
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
    std::cout << total_main_ins+1 << ". "<<INS_Disassemble(ins) << std::endl;
    ++total_main_ins;

    /* Checking for user defined procedure call instructions */
    if(INS_IsProcedureCall(ins))
    {
        std::string call_ins = INS_Disassemble(ins);
        std::string buf;
        std::stringstream ss(call_ins);

        std::vector<std::string> tokens;

        while(ss >> buf)
            tokens.push_back(buf);

        /* Code to convert string to hex */        
        UINT32 proc_ip;
        std::stringstream temp_ss;
        temp_ss << std::hex << tokens[1];
        temp_ss >> proc_ip;


        /* stub to instrument the function and check it for dependency analysis */
        INS_InsertPredicatedCall(
             ins, IPOINT_BEFORE, (AFUNPTR)ProcedureCall,
             IARG_UINT32,proc_ip,
             IARG_END);


    }


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

VOID ProcedureCall(UINT32 ip)
{
    std::cout << "********User defined procedure ********\n";
    std::string proc_name = RTN_FindNameByAddress(ip);
    std::cout << proc_name << std::endl;

    /* To prevent the error : Function RTN_FindByAddress called without holding lock */
    PIN_LockClient();
    RTN rtn = RTN_FindByAddress(ip);

    if (RTN_Valid(rtn) && proc_name != ".plt")  // ".plt"  are inbuilt functions (like printf). No need to analyse
    {        
        RTN_Open(rtn);

        for(INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
        {
            MainInstruction(ins);
        }

        RTN_Close(rtn);
    }
    /* Release the lock */
    PIN_UnlockClient();
}


VOID Routine(RTN rtn, VOID *v)
{
    
    /* Analysing only the main function of a program(start point) */
    if (RTN_Valid(rtn) && RTN_Name(rtn)=="main")
    {        
  
        RTN_Open(rtn);

        for(INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
        {
            MainInstruction(ins);
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
    for(std::map< VOID*,vector<int> >::iterator iter=memoryWriteMap.begin();iter!=memoryWriteMap.end();iter++)
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
    /* Construction on Node Dependency Matrix */

    ndm = (int **)malloc(total_main_ins * sizeof(int *));
    for (int i = 0; i < total_main_ins; i++)
        ndm[i] = (int *)malloc(total_main_ins * sizeof(int));

    std::cout << "Dependency map size " << dependencyMap.size() << std::endl;
    for(std::map< int,vector<int> >::iterator iter=dependencyMap.begin();iter!=dependencyMap.end();iter++)
    {
        // ndm[iter->first-1][iter->second-1] = 1;
        std::cout << iter->first << "'s dependency vector size is " << (iter->second).size() << std::endl;
        for(UINT32 i=0; i < (iter->second).size(); i++)
        {            
            ndm[iter->first-1][(iter->second)[i]-1] = 1;
            std::cout << iter->first <<" depends on " << (iter->second)[i] << std::endl;
        }
    }

    std::cout << "Total main instructions: " << total_main_ins << std::endl;

    std::cout << "\n\n ************** Final NDM ************** \n\n   ";

    for(int i=0; i < total_main_ins; i++)
        std::cout << i+1 << " ";
    std::cout << endl;
    for(int i=0; i < total_main_ins; i++)
    {
        if(i<9)
            std::cout << i+1 << "  ";
        else
            std::cout << i+1 << " ";

        for(int j=0; j < total_main_ins; j++)
        {
            if(j<9)
                std::cout << ndm[i][j] << " ";
            else
                std::cout << ndm[i][j] << "  ";
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