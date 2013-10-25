/*******************************************************
                          cache.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "cache.h"
using namespace std;

Cache::Cache()
{

}

void Cache::Setup(int s,int a,int b,int proto, int processors)
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;
   memoryTransactions = cacheToCacheTransfers = 0;

   protocol = proto;
   num_processors = processors;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   //*******************//
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
      tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
      cache[i][j].invalidate();
      }
   }      
   
}

Cache::Cache(int s,int a,int b )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   //*******************//
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }      
   
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr, uchar op, Cache* cachesArray, int processor_number)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL)/*miss*/
	{
		if(op == 'w') {
         writeMisses++;
         if (protocol == 0) {
            //writeMissFirefly();
         } else {
            writeMissDragon(cachesArray, addr, processor_number);
         }
      }
      /*
      Protocol specific function call
      call writeMissFirefly or writeMissDragon
      Potentially only differentiate when they are different
      */

      /*
      Before or after you process CPU actions,
      process other processors' bus actions
      BusRd & BusUpd methods for bus
      RdMiss, RdHit, WrMiss, WrHit, methods for processors
      */
		else {
         readMisses++;
         if (protocol == 0) {
            //readMissFirefly();
         } else {
            readMissDragon(cachesArray, addr, processor_number);
         }
      }

		//cacheLine *newline = fillLine(addr);
   		//if(op == 'w') newline->setFlags(DIRTY);    
		
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(op == 'w') {
         writeHitDragon(cachesArray, line, addr, processor_number);
      } else {

      }
	}
}

cacheLine* Cache::readMissDragon(Cache* cachesArray, ulong addr, int processor_number) {
   cacheLine *line = BusRd(cachesArray, addr, processor_number);

   cacheLine *newLine = fillLine(addr);
   if (line) {
      newLine->setState(2);
      cacheToCacheTransfers++;
   }
   else {
      newLine->setState(0);
      memoryTransactions++;
   }
   return newLine;
}

cacheLine* Cache::writeMissDragon(Cache* cachesArray, ulong addr, int processor_number) {
   cacheLine *line = BusRd(cachesArray, addr, processor_number);

   cacheLine *newLine = fillLine(addr);
   if (line) {
      newLine->setState(3);
      cacheToCacheTransfers++;
      BusUpd(cachesArray, addr, processor_number);
   }
   else {
      newLine->setState(1);
      memoryTransactions++;
   }
   return newLine;
}

cacheLine* Cache::writeHitDragon(Cache* cachesArray, cacheLine *line, ulong addr, int processor_number) {
   ulong state = line->getState();
   if (state == 0) {
      line->setState(1);
   } else if (state == 2) {
      cacheLine *newLine = BusRd(cachesArray, addr, processor_number);
      if (newLine != NULL) {
         line->setState(3);
      } else {
         line->setState(1);
      }
   } else if (state == 3) {
      cacheLine *newLine = BusRd(cachesArray, addr, processor_number);
      if (newLine == NULL) {
         line->setState(1);
      }
   }
   return line;
}

cacheLine* Cache::BusRd(Cache* cachesArray, ulong addr, int processor_number) {
   cacheLine *line = NULL;
   for (int i = 0; i < num_processors; i++) {
      line = cachesArray[i].findLine(addr);
      if (line != NULL) {
         ulong state = line->getState();
         if (state == 0)
            line->setState(2);
         if (state == 1)
            line->setState(3);
         return line;
      }
   }
   
   return line;
}

cacheLine* Cache::BusUpd(Cache* cachesArray, ulong addr, int processor_number) {
   cacheLine *line = NULL;
   for (int i = 0; i < num_processors; i++) {
      line = cachesArray[i].findLine(addr);
      if (line != NULL) {
         ulong state = line->getState();
         if (state == 3)
            line->setState(2);
         return line;
      }
   }
   return line;
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if(victim->getState() == 1 || victim->getState() == 3) {
      writeBack(addr);
      memoryTransactions++;
   }
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats()
{ 
	//printf("===== Simulation results      =====\n");
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
   double missRate = (double)(readMisses + writeMisses) / (reads + writes);
   //printf("%50s%lf\n", "05. total miss rate:", missRate);
   printf("%-50s%lu\n", "01. number of reads:", reads);
   printf("%-50s%lu\n", "02. number of read misses:", readMisses);
   printf("%-50s%lu\n", "03. number of writes:", writes);
   printf("%-50s%lu\n", "04. number of write misses:", writeMisses);
   printf("%-50s%lf\n", "05. total miss rate:", missRate);
   printf("%-50s%lu\n", "06. number of writebacks:", writeBacks);
   printf("%-50s%lu\n", "07. number of memory transactions:", memoryTransactions);
   printf("%-50s%lu\n", "08. number of cache to cache transfers:", cacheToCacheTransfers);
}
