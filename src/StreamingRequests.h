/*
 * StreamingRequests.h
 *
 *  Created on: Nov 1, 2017
 *      Author: x300
 */

#ifndef STREAMINGREQUESTS_H_
#define STREAMINGREQUESTS_H_

#define NUM_REQUESTS_QUE 256


class StreamingRequests {

	long long int NumSamples[NUM_REQUESTS_QUE];
	int PtrRd = 0;
	int PtrWr = 0;

public:
	StreamingRequests();
	virtual ~StreamingRequests();
	void InsertRequest(long long int ValIn);
	long long int GetRequest(void);
};

#endif /* STREAMINGREQUESTS_H_ */
