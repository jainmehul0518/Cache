#include "cache.h"

cache::cache()
{
	for (int i=0; i<L1_CACHE_SETS; i++)
		L1[i].valid = false; 
	for (int i=0; i<L2_CACHE_SETS; i++)
		for (int j=0; j<L2_CACHE_WAYS; j++)
			L2[i][j].valid = false; 

	this->myStat.missL1 =0;
	this->myStat.missL2 =0;
	this->myStat.accL1 =0;
	this->myStat.accL2 =0;
}

void cache::controller(bool MemR, bool MemW, int* data, int adr, int* myMem)
{
	bool isFoundInL1 = false;
	bool isFoundInL2 = false;

	// calculate index and tag for both L1 and L2 caches
	int index = adr & 15;
	int tag = adr & 4294967280;
	tag >>= 4;
	int way;
	int dataFromL2;
	int dataFromL1;
	int dataFromMem;
	int tagFromL2;
	int tagFromL1;
	int tagFromMem;

	// first search L1 cache
	isFoundInL1 = searchL1(index, tag);
	if (MemW) // MemW logic for L1 cache
	{
		if (!isFoundInL1) // L1 cache miss
		{
			myStat.missL1+=1;
			isFoundInL2 = searchL2(index, tag, way);
			if (!isFoundInL2)
				myStat.missL2+=1;
			else
			{
				// update L2 line data with new value
				updateL2(index, tag, way, *data);
				// evict L2 line
				evictFromL2(index, way, dataFromL2, tagFromL2);
				// attempt to evict L1 line
				bool evictedFromL1 = evictFromL1(index, dataFromL1, tagFromL1);
				// update L1 line with L2 line
				updateL1(index, tagFromL2, dataFromL2);
				// if able to evict line from L1, store in L2
				if (evictedFromL1)
					updateL2(index, tagFromL1, -1, dataFromL1);
			}
		}
		else // L1 cache hit
			updateL1(index, tag, *data);
		// regardless of caches' hit or miss, update memory
		myMem[adr] = *data;
	}
	else // MemR logic for L1 cache
	{
		if (!isFoundInL1) // L1 cache miss
		{
			myStat.missL1+=1;
			isFoundInL2 = searchL2(index, tag, way);
			if (!isFoundInL2)
			{
				myStat.missL2+=1;
				// evict L1 line
				bool evictedFromL1 = evictFromL1(index, dataFromL1, tagFromL1);
				// bring memory data to L1
				updateL1(index, tag, myMem[adr]);
				// if able to evict line from L1, store in L2
				if (evictedFromL1)
				{
					// first evict LRU line from L2
					evictFromL2(index, -1, dataFromL2, tagFromL2);
					// now place old L1 line into L2
					updateL2(index, tagFromL1, -1, dataFromL1);
				}
			}
			else
			{
				// evict L2 line
				evictFromL2(index, way, dataFromL2, tagFromL2);
				// attempt to evict L1 line
				bool evictedFromL1 = evictFromL1(index, dataFromL1, tagFromL1);
				// update L1 line with L2 line
				updateL1(index, tagFromL2, dataFromL2);
				// if able to evict line from L1, store in L2
				if (evictedFromL1)
					updateL2(index, tagFromL1, -1, dataFromL1);
			}
		}
	}
}

// output current L1 and L2 cache contents
void cache::printCacheInstance()
{
	cout << "L1: " << endl;
	for (int i=0; i<L1_CACHE_SETS; i++)
		cout << "Valid: " << L1[i].valid << " Tag: " << L1[i].tag << " Data: " << L1[i].data << endl;
	cout << endl;
	cout << "L2: " << endl;
	for (int i=0; i<L2_CACHE_SETS; i++)
	{
		for (int j=0; j<L2_CACHE_WAYS; j++)
			cout << "Valid: " << L2[i][j].valid << " Tag: " <<  L2[i][j].tag << " Data: " << L2[i][j].data << " LRU: " << L2[i][j].lru_position << ", ";
		cout << endl;
	}
}

// search for address in L1
bool cache::searchL1(int index, int tag)
{
	myStat.accL1 += 1; // keep track of L1 accesses
	return (L1[index].valid && L1[index].tag == tag);
}

// insert address in L1
void cache::updateL1(int index, int tag, int data)
{
	L1[index].valid = 1;
	L1[index].tag = tag;
	L1[index].data = data;
}

// search for address in L2
bool cache::searchL2(int index, int tag, int& way_res)
{
	myStat.accL2 += 1; // keep track of L2 accesses
	for (int way = 0; way < L2_CACHE_WAYS; way++)
	{
		if (L2[index][way].valid && L2[index][way].tag == tag)
		{
			way_res = way;
			return true;
		}
	}
	return false;
}

// insert address in L2
void cache::updateL2(int index, int tag, int way, int data)
{
	int old_lru_position = 0;
	if (way >= 0)
	{
		L2[index][way].valid = 1;
		L2[index][way].tag = tag;
		L2[index][way].data = data;
		old_lru_position = L2[index][way].lru_position;
		L2[index][way].lru_position = 8;
	}
	else
	{
		for (int i = 0; i < L2_CACHE_WAYS; i++)
		{
			if (L2[index][i].valid == 0)
			{
				L2[index][i].valid = 1;
				L2[index][i].tag = tag;
				L2[index][i].data = data;
				L2[index][i].lru_position = 8;
				break;
			}
		}
	}
	for (int i = 0; i < L2_CACHE_WAYS; i++)
	{
		if (L2[index][i].lru_position > old_lru_position)
			L2[index][i].lru_position -= 1;
	}
}

// remove adress from L2
void cache::evictFromL2(int index, int way, int& data, int& tag)
{
	if (way < 0) // means that we have to check if the current set is full. If so, evict oldest entry
	{
		for (int i = 0; i < L2_CACHE_WAYS; i++)
		{
			if (L2[index][i].valid == 1 && L2[index][i].lru_position == 0)
			{
				L2[index][i].valid = 0;
				break;
			}
		}
	}
	else // evict entry at specified way and update remaining LRU values for current set
	{
		data = L2[index][way].data;
		tag = L2[index][way].tag;
		L2[index][way].valid = 0;
		for (int i = 0; i < L2_CACHE_WAYS; i++)
		{
			if (L2[index][i].lru_position < L2[index][way].lru_position)
				L2[index][i].lru_position += 1;
		}
	}
}

// evict entry from L1
bool cache::evictFromL1(int index, int& data, int& tag)
{
	if (!L1[index].valid)
		return false;
	data = L1[index].data;
	tag = L1[index].tag;
	L1[index].valid = 0;
	return true;
}

void cache::computeStats(float& L1_miss_rate, float& L2_miss_rate, float& AAT)
{
	L1_miss_rate = static_cast<float>(myStat.missL1)/myStat.accL1;
	L2_miss_rate = static_cast<float>(myStat.missL2)/myStat.accL2;
	AAT = 1 + (L1_miss_rate * (8 + (L2_miss_rate * 100)));
}
