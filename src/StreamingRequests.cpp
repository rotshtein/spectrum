/*
 * StreamingRequests.cpp
 *
 *  Created on: Nov 1, 2017
 *      Author: x300
 */

#include "StreamingRequests.h"

StreamingRequests::StreamingRequests() {
	// TODO Auto-generated constructor stub

}

StreamingRequests::~StreamingRequests() {
	// TODO Auto-generated destructor stub
}


void StreamingRequests::InsertRequest(long long int ValIn)
{
	NumSamples[PtrWr] = ValIn;
	int OldPtr = PtrWr;
	OldPtr++;
	OldPtr &= (NUM_REQUESTS_QUE-1);
	PtrWr = OldPtr;
}

long long int StreamingRequests::GetRequest(void)
{
	if (PtrWr == PtrRd) {
		return -1;
	}

	long long int RetVal = NumSamples[PtrRd];

	int OldPtr = PtrRd;
	OldPtr++;
	OldPtr &= (NUM_REQUESTS_QUE - 1);
	PtrRd = OldPtr;
	return RetVal;

}
