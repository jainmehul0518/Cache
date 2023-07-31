#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for SA only
	int data; // the actual data stored in the cache/memory
	bool valid;
	// add more things here if needed
};

struct Stat
{
	int missL1; 
	int missL2; 
	int accL1;
	int accL2;
	// add more stat if needed. Don't forget to initialize!
};

class cache {
private:
	cacheBlock L1[L1_CACHE_SETS]; // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row 
	Stat myStat;
	// add more things here
public:
	cache();
	void controller(bool MemR, bool MemW, int* data, int adr, int* myMem);
	/*
	bool checkL1(int* data, int adr,int* myMem);
	bool checkL2(int* data, int adr, int* myMem);
	*/
	bool searchL1(int index, int tag);
	void updateL1(int index, int tag, int data);
	bool evictFromL1(int index, int& data, int& tag);
	bool searchL2(int index, int tag, int& way);
	void updateL2(int index, int tag, int way, int data);
	void evictFromL2(int index, int way, int& data, int& tag);
	void printCacheInstance();
	void computeStats(float& L1_miss_rate, float& L2_miss_rate, float& AAT);
	// add more functions here ...	
};


