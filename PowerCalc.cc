/*
 * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
 * All rights reserved.
 *
 * Licensed under BSD 3-Clause License
 *
 * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
 *
 */

#include "src/MemorySpecification.h"
#include "src/MemoryPowerModel.h"
#include <iostream>
#include <ctime>
#include "src/CmdScheduler.h"
#include "src/MemSpecParser.h"
#include <math.h>

using namespace Data;
using namespace std;

void error(){
    std::cout << "Correct Usage: \n./drampower -m <memory spec (ID)> "
            "[-t] <transactions trace> [-c] <commands trace> [-i] "
			"<interleaving> [-g] <DDR4 bank group "
			"interleaving> [-s] <request size> [-r] "
			"[-p] < 1 - Power-Down, 2 - Self-Refresh>\n";
}

int main(int argc, char* argv[]) {

    unsigned trans = 0, cmds = 0, memory = 0, size = 0, term = 0, power_down= 0;

    char* src_trans = {0};
    char* src_cmds = {0};
    char* src_memory = {0};

    unsigned interleaving = 1, grouping = 1, src_size = 1, burst = 1;

    for (int i = 1; i < argc; i++) {
        if (i + 1 != argc) {
            if (string(argv[i]) == "-t") {
                src_trans = argv[i + 1];
                trans = 1;
            } else if (string(argv[i]) == "-c") {
                src_cmds = argv[i + 1];
                cmds = 1;
            } else if (string(argv[i]) == "-m") {
                src_memory = argv[i + 1];
                memory = 1;
            } else if (string(argv[i]) == "-i") {
                interleaving = atoi(argv[i + 1]);
            } else if (string(argv[i]) == "-g") {
                grouping = atoi(argv[i + 1]);
            } else if (string(argv[i]) == "-s") {
                src_size = atoi(argv[i + 1]);
                size = 1;
            } else if (string(argv[i]) == "-p") {
                power_down = atoi(argv[i + 1]);
            } else {
				if (string(argv[i]) == "-r")
		    		term = 1;
                continue;
            }
        } else {
			if (string(argv[i]) == "-r")
		    	term = 1;
            continue;
		}
    }

    if(memory == 0){
        cout << endl << "No DRAM memory specified!" << endl;
        error();
        return 0;
    }

    ifstream fout;
    if(trans)
    {
        fout.open(src_trans);
        if(fout.fail()){
            cout<<"Transactions trace file not found!"<<endl;
            error();
            return 0;
        }
    }
    else
    {
        fout.open(src_cmds);
        if(fout.fail()){
            cout<<"Commands trace file not found!"<<endl;
            error();
            return 0;
        }
    }
    fout.close();

    //Replace the memory specification XML file with another in the same format
    //from the memspecs folder
    MemorySpecification memSpec(MemorySpecification::
            getMemSpecFromXML(src_memory));

    MemArchitectureSpec& memArchSpec = memSpec.memArchSpec;

    if(interleaving > memArchSpec.nbrOfBanks)
    {
        cout << "Interleaving > Number of Banks" << endl;
        error();
        return 0;
    }

    if(grouping > memArchSpec.nbrOfBankGroups)
    {
        cout << "Grouping > Number of Bank Groups" << endl;
        error();
        return 0;
    }

    if(power_down > 2)
    {
        cout << "Incorrect power-down option" << endl;
        error();
        return 0;
    }

    unsigned min_size = interleaving * grouping * memArchSpec.burstLength
													* memArchSpec.width / 8;

    if(size == 0){
        src_size = min_size;
    } else{
        src_size = max(min_size, src_size);
    }

    burst = src_size / min_size;
    //transSize = BGI * BI * BC * BL.

    const clock_t begin_time = clock();

    ifstream trace_file;

    if (trans) {
        trace_file.open(src_trans, ifstream::in);
    } else if (cmds) {
        trace_file.open(src_cmds, ifstream::in);
    } else {
        cout << "No transaction or command trace file specified!" << endl;
        error();
        return 0;
    }

    MemoryPowerModel mpm;

    time_t start = time(0);
    tm* starttm = localtime(&start);
    cout << "* Analysis start time: " << asctime(starttm);
    cout << "* Analyzing the input trace" << endl;

    //Calculates average power consumption and energy for the input memory
    //command trace
    mpm.trace_power(memSpec, trace_file, trans, grouping, interleaving, burst,
							     					 		term, power_down);

    time_t end = time(0);
    tm* endtm = localtime(&end);
    cout << "* Power Computation End time: " << asctime(endtm);

    cout << "* Total Simulation time: " << float(clock() - begin_time) /
                                          CLOCKS_PER_SEC << " seconds" << endl;

    return 0;
}
