#include "MobCounter.h"
CMobCounter::CMobCounter()
{
	ZeroMemory(cNpcName, sizeof(cNpcName));
	iKillCount = 0;
	iLevel = 0;
	iNextCount = 0;
}


CMobCounter::~CMobCounter()
{
}