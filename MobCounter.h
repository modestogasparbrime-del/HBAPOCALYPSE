#pragma once
#include <windows.h>
class CMobCounter
{
public:
	CMobCounter();
	virtual ~CMobCounter();

	char cNpcName[21];
	int iKillCount, iLevel, iNextCount;
};

