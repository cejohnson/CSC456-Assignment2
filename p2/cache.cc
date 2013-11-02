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

   /**create a two dimensional cache, sized as cache[sets][assoc]**/
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++)
      {
      cache[i][j].setTag(0);
      }
   }

}

Cache::Cache(int s,int a,int b )
{
   ulong i;//, j;
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
      /*for(j=0; j<assoc; j++)
      {
	   cache[i][j].invalidate();
      }*/
   }

}

/**you might add other parameters to Access()
since this function is an entry point
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr, uchar op, Cache* cachesArray, int processor_number) {
   currentCycle++;/*per cache global counter to maintain LRU order
         among cache ways, updated on every cache access*/

   if(op == 'w') writes++;
   else          reads++;

   cacheLine * line = findLine(addr);
   if(line == NULL) {/*miss*/
      if(op == 'w') { //write miss
         writeMisses++;
         PrWrMiss(cachesArray, addr, processor_number);
      } else { //read miss
         readMisses++;
         PrRdMiss(cachesArray, addr, processor_number);
      }
   } else { //in cache
      /**since it's a hit, update LRU and update dirty flag**/
      if(op == 'w') { //write hit
         PrWr(cachesArray, line, addr, processor_number);
      } else { //read hit
        //Line in cache
      }
      updateLRU(line);
   }
}

cacheLine* Cache::PrRdMiss(Cache* cachesArray, ulong addr, int processor_number) {
   /* BusRd: if it returns a line, then it is shared, otherwise not */
   cacheLine *sharedLine = BusRd(cachesArray, addr, processor_number);
   cacheLine *newLine = fillLine(addr);

   if (protocol == 0) { //Firefly
      if (sharedLine) { //in other cache
         newLine->setState(SHARED);
         cacheToCacheTransfers++;
      } else { //not in any cache
         newLine->setState(VALID);
         memoryTransactions++;
      }
   }
   else { //Dragon
      if (sharedLine) {
         newLine->setState(SHARED_CLEAN);
         cacheToCacheTransfers++;
      }
      else {
         newLine->setState(EXCLUSIVE);
         memoryTransactions++;
      }
   }
   return newLine;
}

cacheLine* Cache::PrWrMiss(Cache* cachesArray, ulong addr, int processor_number) {
   //cacheLine *sharedLine = BusRd(cachesArray, addr, processor_number);
   //cacheLine *newLine = fillLine(addr);
   cacheLine *newLine = NULL;

   if (protocol == 0) { //Firefly
      /*if (sharedLine) { //in other cache
         newLine->setState(SHARED);
         cacheToCacheTransfers++;
         memoryTransactions++;
      } else { //not in any cache
         newLine->setState(DIRTY);
         memoryTransactions++;
      }*/
      newLine = PrRdMiss(cachesArray, addr, processor_number);
      PrWr(cachesArray, newLine, addr, processor_number);
   }
   else { //Dragon
      cacheLine *sharedLine = BusRd(cachesArray, addr, processor_number);
      cacheLine *newLine = fillLine(addr);
      if (sharedLine) { //another cache has it
         newLine->setState(SHARED_MODIFIED);
         cacheToCacheTransfers++;
         BusUpd(cachesArray, addr, processor_number);
      }
      else { //no cache has it
         newLine->setState(MODIFIED);
         memoryTransactions++;
      }
   }
   return newLine;
}

cacheLine* Cache::PrWr(Cache* cachesArray, cacheLine *line, ulong addr, int processor_number) {
   ulong state = line->getState();
   if (protocol == 0) {
      if (state == VALID) {
         line->setState(DIRTY);
      } else if (state == SHARED) {
         //Update all copies, write to memory, if only copy, set Valid
         cacheLine *sharedLine = BusUpd(cachesArray, addr, processor_number);
         if (sharedLine == NULL) {
            line->setState(VALID);
         }

         memoryTransactions++;
      } else if (state == DIRTY) {
         //Update cache, no change state
      } else {
	 printf("Unaccounted for prwr state");
      }
   }
   else {
      if (state == EXCLUSIVE) {
         line->setState(MODIFIED);
      } else if (state == SHARED_CLEAN) {
         cacheLine *sharedLine = BusUpd(cachesArray, addr, processor_number);
         if (sharedLine != NULL) {
            line->setState(SHARED_MODIFIED);
         } else {
            line->setState(MODIFIED);
         }
      } else if (state == SHARED_MODIFIED) {
         cacheLine *sharedLine = BusUpd(cachesArray, addr, processor_number);
         if (sharedLine == NULL) {
            line->setState(MODIFIED);
         }
      }
   }

   return line;
}

/*
 * Iterates through the caches and searches each one for the address passed in
 * If the line is found, its state is updated and foundLine is set to this line
 * Returns foundLine, which will be NULL if no lines were found
 */
cacheLine* Cache::BusRd(Cache* cachesArray, ulong addr, int processor_number) {
   cacheLine *line = NULL; // Temp variable
   cacheLine *foundLine = NULL; // Found line
   for (int i = 0; i < num_processors; i++) {
    if (i == processor_number) {
      if ((++i) == num_processors) continue;
    }



      line = cachesArray[i].findLine(addr);
      if (line != NULL) { // Update state based on

         if (protocol == 0) {
           ulong state = line->getState();
           if (state == VALID) {
             line->setState(SHARED);
           } else if (state == DIRTY) {
             line->setState(SHARED);
             memoryTransactions++; // Update main memory
           }
         }
         else {
            ulong state = line->getState();
            if (state == MODIFIED) {
               line->setState(SHARED_MODIFIED);
            } else if (state == EXCLUSIVE) {
               line->setState(SHARED_CLEAN);
            }
         }
         foundLine = line;
      }

      //Skip the processor who started the BusRd
//      if (i+1 == processor_number) i++;
   }

   return foundLine;
}

cacheLine* Cache::BusUpd(Cache* cachesArray, ulong addr, int processor_number) {
   cacheLine *line = NULL;
   cacheLine *foundLine = NULL;
   for (int i = 0; i < num_processors; i++) {
    if (i == processor_number) {
      if ((++i) == num_processors) continue;
    }



      line = cachesArray[i].findLine(addr);
      if (line != NULL) {
        if (protocol == 0) {
           line->setState(SHARED);
        } else {
            ulong state = line->getState();
            if (state == SHARED_MODIFIED) {
               line->setState(SHARED_CLEAN);
            }
         }
         foundLine = line;
      }

      //Skip the processor who started the BusRd
//      if (i+1 == processor_number) i++;
   }
   return foundLine;
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;

   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);

   for(j=0; j<assoc; j++) {
	//if(cache[i][j].isValid())
	  if(cache[i][j].getTag() == tag)
		{
		     pos = j; break;
		}
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

   /*for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);
   }*/
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
   if (protocol == 0) {
      if (victim->getState() == DIRTY) {
         writeBacks++;
         memoryTransactions++;
      }
   }
   else {
      if(victim->getState() == MODIFIED) {
         writeBacks++;
         memoryTransactions++;
      } else if (victim->getState() == SHARED_MODIFIED) {
         writeBacks++;
         memoryTransactions++;
      }

   }
   tag = calcTag(addr);
   victim->setTag(tag);
   //victim->setFlags(VALID);
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

   printf("%-50s%lu\n", "01. number of reads:", reads);
   printf("%-50s%lu\n", "02. number of read misses:", readMisses);
   printf("%-50s%lu\n", "03. number of writes:", writes);
   printf("%-50s%lu\n", "04. number of write misses:", writeMisses);
   printf("%-50s%lf\n", "05. total miss rate:", missRate);
   printf("%-50s%lu\n", "06. number of writebacks:", writeBacks);
   printf("%-50s%lu\n", "07. number of memory transactions:", memoryTransactions);
   printf("%-50s%lu\n", "08. number of cache to cache transfers:", cacheToCacheTransfers);
}
