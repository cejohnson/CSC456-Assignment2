/*******************************************************
                          main.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
#include <string.h>
using namespace std;

#include "cache.h"

int main(int argc, char *argv[]){
	ifstream fin;
	FILE * pFile;

	if(argv[1] == NULL){
		 printf("input format: ");
		 printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
		 exit(0);
        }

	/*****uncomment the next five lines*****/
	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:MOESI*/
	char *pname = (char *)malloc(20);
	if (protocol == 0)
		pname = strcpy(pname, "Firefly");
	else
		pname = strcpy(pname, "Dragon");
	char *fname = (char *)malloc(20);
 	fname = argv[6];


	//****************************************************//
	printf("===== Simulator Configuration =====\n");
	//*******print out simulator configuration here*******//
	printf("%-23s%d\n", "L1_SIZE:", cache_size);
	printf("%-23s%d\n", "L1_ASSOC:", cache_assoc);
	printf("%-23s%d\n", "L1_BLOCKSIZE:", blk_size);
	printf("%-23s%d\n", "NUMBER OF PROCESSORS:", num_processors);
	printf("%-23s%s\n", "COHERENCE PROTOCOL:", pname);
	printf("%-23s%s\n", "TRACE FILE:", fname);
	//****************************************************//


	//*********************************************//
        //*****create an array of caches here**********//
	Cache* cachesArray = new Cache[num_processors];// = new Cache[num_processors];
	for (int i = 0; i < num_processors; i++) {
		cachesArray[i].Setup(cache_size, cache_assoc, blk_size, protocol, num_processors);
	}
	//*********************************************//

	pFile = fopen (fname,"r");
	if(pFile == 0)
	{
		printf("Trace file problem\n");
		exit(0);
	}
	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	///******************************************************************//

	/* Provided by Dr. Gehringer to do some I/O */
	int processor_number;
	uchar op;
	ulong address;

	while (fscanf(pFile, "%i %c %lx", &processor_number, &op, &address) == 3) {
		cachesArray[processor_number].Access(address, op, cachesArray, processor_number);
	}


	fclose(pFile);

	//********************************//
	//print out all caches' statistics //
	//********************************//

	for (int i = 0; i < num_processors; i++) {
		printf("===== Simulation results (Cache_%i)      =====\n", i);
		cachesArray[i].printStats();
	}

	delete[] cachesArray;
	free(pname);
}
