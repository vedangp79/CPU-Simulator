/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <stdio.h>
#include <stdlib.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

// **Note** this is a preprocessor macro. This is not the same as a function.
// Powers of 2 have exactly one 1 and the rest 0's, and 0 isn't a power of 2.
#define is_power_of_2(val) (val && !(val & (val - 1)))


/*
 * Accesses 1 word of memory.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads and 1 for writes.
 * write_data is a word, and is only valid if write_flag is 1.
 * If write flag is 1, mem_access does: state.mem[addr] = write_data.
 * The return of mem_access is state.mem[addr].
 */
extern int mem_access(int addr, int write_flag, int write_data);

/*
 * Returns the number of times mem_access has been called.
 */
extern int get_num_mem_accesses(void);

//Use this when calling printAction. Do not modify the enumerated type below.
enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

/* You may add or remove variables from these structs */
typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int dirty;
    int lruLabel;
    int tag;
    int valid;
    int set;
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printAction(int, int, enum actionType);
void printCache(void);

/*
 * Set up the cache with given command line parameters. This is
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet)
{
    if (blockSize <= 0 || numSets <= 0 || blocksPerSet <= 0) {
        printf("error: input parameters must be positive numbers\n");
        exit(1);
    }
    if (blocksPerSet * numSets > MAX_CACHE_SIZE) {
        printf("error: cache must be no larger than %d blocks\n", MAX_CACHE_SIZE);
        exit(1);
    }
    if (blockSize > MAX_BLOCK_SIZE) {
        printf("error: blocks must be no larger than %d words\n", MAX_BLOCK_SIZE);
        exit(1);
    }
    if (!is_power_of_2(blockSize)) {
        printf("warning: blockSize %d is not a power of 2\n", blockSize);
    }
    if (!is_power_of_2(numSets)) {
        printf("warning: numSets %d is not a power of 2\n", numSets);
    }
    printf("Simulating a cache with %d total lines; each line has %d words\n",
        numSets * blocksPerSet, blockSize);
    printf("Each set in the cache contains %d lines; there are %d sets\n",
        blocksPerSet, numSets);

    cache.blockSize = blockSize;
    cache.numSets = numSets;
    cache.blocksPerSet = blocksPerSet;

    for (int i = 0; i < cache.numSets; ++i) {
        for (int j = 0; j < cache.blocksPerSet; ++j) {
            cache.blocks[i * cache.blocksPerSet + j].dirty = 0;
            cache.blocks[i * cache.blocksPerSet + j].valid = 0; 
            cache.blocks[i * cache.blocksPerSet + j].tag = -1;
            cache.blocks[i * cache.blocksPerSet + j].set = i;
            cache.blocks[i * cache.blocksPerSet + j].lruLabel = 0;
        }
    }
    // Your code here

    return;
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */

 int LRU_index(cacheStruct cache, int set);

 int cache_access(int addr, int write_flag, int write_data) {
    int block_offset = addr % cache.blockSize;
    int set = (addr / cache.blockSize) % cache.numSets;
    int tag = addr / cache.blockSize / cache.numSets;
    // lw
    if (write_flag == 0) {
        // check for hit
        int block_ind = set * cache.blocksPerSet;
        while (block_ind < (set + 1) * cache.blocksPerSet && tag != cache.blocks[block_ind].tag) {
            block_ind++;
        }
        // hit
        if (block_ind < (set + 1) * cache.blocksPerSet) {
            printAction(addr, 1, cacheToProcessor);
            // update lru
            for (int i = set * cache.blocksPerSet; i < (set + 1) * cache.blocksPerSet; i++) {
                if (cache.blocks[i].lruLabel < cache.blocks[block_ind].lruLabel) {
                    cache.blocks[i].lruLabel++;
                }
            }
            cache.blocks[block_ind].lruLabel = 0;          
            return cache.blocks[block_ind].data[block_offset];
        }
        // miss
        else {
            // load block from mem
            // check if eviction needed
            int emptyBlock = set * cache.blocksPerSet;
            while (emptyBlock < (set + 1) * cache.blocksPerSet && cache.blocks[emptyBlock].valid) {
                emptyBlock++;
            }
            // eviction needed
            if (emptyBlock == (set + 1) * cache.blocksPerSet) {
                int lruBlock = LRU_index(cache, set);
                int blockAddr = ((cache.blocks[lruBlock].tag * cache.numSets) + cache.blocks[lruBlock].set) * cache.blockSize;
                // dirty bit = 1 means should be evicted
                if (cache.blocks[lruBlock].dirty) {
                    // update mem with the needing evicted lru block
                    for (int i = 0; i < cache.blockSize; ++i) {
                        int currAddr = blockAddr + i;
                        mem_access(currAddr, 1, cache.blocks[lruBlock].data[i]);
                    }
                    printAction(blockAddr, cache.blockSize, cacheToMemory);
                }
                // lru block not dirty
                else {
                    printAction(blockAddr, cache.blockSize, cacheToNowhere);
                }
                // update cache: replace the lru block from mem
                for (int j = 0; j < cache.blockSize; ++j) {
                    cache.blocks[lruBlock].data[j] = mem_access(addr - block_offset + j, 0, write_data);
                }
                cache.blocks[lruBlock].tag = tag;
                cache.blocks[lruBlock].valid = 1;
                cache.blocks[lruBlock].dirty = 0;
                printAction(addr - block_offset, cache.blockSize, memoryToCache);
                // update lru
                for (int i = set * cache.blocksPerSet; i < (set + 1) * cache.blocksPerSet; ++i) {
                    cache.blocks[i].lruLabel++;
                }
                cache.blocks[lruBlock].lruLabel = 0;  
                // return cache line data
                printAction(addr, 1, cacheToProcessor);
                return cache.blocks[lruBlock].data[block_offset];                  
            }
            // exist empty blocks in the set
            else {
                // read from mem
                for (int i = 0; i < cache.blockSize; ++i) {
                    cache.blocks[emptyBlock].data[i] = mem_access(addr - block_offset + i, 0, write_data);
                }
                cache.blocks[emptyBlock].tag = tag;
                cache.blocks[emptyBlock].valid = 1;
                cache.blocks[emptyBlock].dirty = 0;
                printAction(addr - block_offset, cache.blockSize, memoryToCache);
                // update lru
                for (int i = set * cache.blocksPerSet; i < (set + 1) * cache.blocksPerSet; ++i) {
                    cache.blocks[i].lruLabel++;
                }
                cache.blocks[emptyBlock].lruLabel = 0;     
                // return cache line data
                printAction(addr, 1, cacheToProcessor);
                return cache.blocks[emptyBlock].data[block_offset];                               
            }
        }            
    }
    // sw
    else {
        // check for hit
        int block_ind = set * cache.blocksPerSet;
        while (block_ind < (set + 1) * cache.blocksPerSet && tag != cache.blocks[block_ind].tag) {
            block_ind++;
        }
        // hit
        if (block_ind < (set + 1) * cache.blocksPerSet) {
            // update cache line data
            cache.blocks[block_ind].data[block_offset] = write_data;
            cache.blocks[block_ind].dirty = 1;
            cache.blocks[block_ind].valid = 1;
            // update lru
            for (int i = set * cache.blocksPerSet; i < (set + 1) * cache.blocksPerSet; ++i) {
                if (cache.blocks[i].lruLabel < cache.blocks[block_ind].lruLabel) {
                    cache.blocks[i].lruLabel++;
                }
            }
            cache.blocks[block_ind].lruLabel = 0;
            // print action
            printAction(addr, 1, processorToCache);
            return 0;
        }
        // miss
        else {
            // check if eviction needed
            int emptyBlock = set * cache.blocksPerSet;
            while (emptyBlock < (set + 1) * cache.blocksPerSet && cache.blocks[emptyBlock].valid) {
                emptyBlock++;
            }
            // eviction needed
            if (emptyBlock == (set + 1) * cache.blocksPerSet) {
                int lruBlock = LRU_index(cache, set);
                int block_Addr = ((cache.blocks[lruBlock].tag * cache.numSets) + cache.blocks[lruBlock].set) * cache.blockSize;
                // if block needs to be evicted has dirty bit = 1.
                if (cache.blocks[lruBlock].dirty) {
                    // update mem with the needing evicted lru block
                    for (int j = 0; j < cache.blockSize; ++j) {
                        int curr_Addr = block_Addr + j;
                        mem_access(curr_Addr, 1, cache.blocks[lruBlock].data[j]);
                    }
                    printAction(block_Addr, cache.blockSize, cacheToMemory);
                }
                // lru block not dirty
                else {
                    printAction(block_Addr, cache.blockSize, cacheToNowhere);
                }
                // update cache: replace the lru block from mem
                for (int j = 0; j < cache.blockSize; ++j) {
                    cache.blocks[lruBlock].data[j] = mem_access(addr - block_offset + j, 0, write_data);
                }
                printAction(addr - block_offset, cache.blockSize, memoryToCache);
                // update cache line data
                cache.blocks[lruBlock].data[block_offset] = write_data;
                cache.blocks[lruBlock].tag = tag;
                cache.blocks[lruBlock].dirty = 1;
                cache.blocks[lruBlock].valid = 1;
                printAction(addr, 1, processorToCache);
                // update lru
                for (int i = set * cache.blocksPerSet; i < (set + 1) * cache.blocksPerSet; ++i) {
                    cache.blocks[i].lruLabel++;
                }
                cache.blocks[lruBlock].lruLabel = 0;  
                return 0;  
            }
            // exist empty blocks in the set
            else {
                // read from mem
                for (int j = 0; j < cache.blockSize; j++) {
                    cache.blocks[emptyBlock].data[j] = mem_access(addr - block_offset + j, 0, write_data);
                }
                printAction(addr - block_offset, cache.blockSize, memoryToCache);                
                // update cache line data
                cache.blocks[emptyBlock].data[block_offset] = write_data;
                cache.blocks[emptyBlock].tag = tag;
                cache.blocks[emptyBlock].dirty = 1;
                cache.blocks[emptyBlock].valid = 1;
                printAction(addr, 1, processorToCache);
                for (int i = set * cache.blocksPerSet; i < (set + 1) * cache.blocksPerSet; ++i) {
                    cache.blocks[i].lruLabel++;
                }
                cache.blocks[emptyBlock].lruLabel = 0;    
                return 0;                
            }
        } 
    }
}

int LRU_index(cacheStruct cache, int set){
    int maximum = cache.blocks[set*cache.blocksPerSet].lruLabel;
    int res = set*cache.blocksPerSet;
    for (int i = set*cache.blocksPerSet + 1; i < (set+1)*cache.blocksPerSet; ++i) {
        if (cache.blocks[i].lruLabel > maximum) {
            maximum = cache.blocks[i].lruLabel;
            res = i;
        }
    }
    return res;
}



/*
 * print end of run statistics like in the spec. **This is not required**,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(void)
{
    return;
}

/*
 * Log the specifics of each cache action.
 *
 *DO NOT modify the content below.
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
    else {
        printf("Error: unrecognized action\n");
        exit(1);
    }

}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache(void)
{
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set) {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block) {
            printf("\t\t[ %i ]: {", block);
            for (int index = 0; index < cache.blockSize; ++index) {
                printf(" %i", cache.blocks[set * cache.blocksPerSet + block].data[index]);
            }
            printf(" }\n");
        }
    }
    printf("end cache\n");
}
