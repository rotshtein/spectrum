/*
 * Streamer.h
 *
 *  Created on: Oct 31, 2017
 *      Author: x300
 */

#ifndef STREAMER_H_
#define STREAMER_H_


#include "Buffers.h"
#include "StreamingRequests.h"
#include "defs.h"

class Streamer {
	double MaxTime;
protected:
	Buffers *Buff1;
	StreamingRequests *ReqQue;
	uhd::usrp::multi_usrp::sptr usrp;
	long long int num_requested_samples;
	unsigned long long LastPrintSample = 0;
	double Rate;
	double diff_time(struct timespec &tp0, struct timespec &tp1)
	{
		double secs0 = tp0.tv_sec;
		double secs1 = tp1.tv_sec;
		double nsecs0 = tp0.tv_nsec;
		double nsecs1 = tp1.tv_nsec;
		double dnsec = nsecs1 - nsecs0;
		dnsec *= 1e-9;
		double difft = secs1-secs0;
		difft += dnsec;
		return difft;


	}

public:
	Streamer(struct StreamerParams *Params);
	virtual ~Streamer();
	virtual void Run(void);
	virtual void SetRate(double Val)
	{
		Rate = Val;
	}
	bool Overflow = false;
	bool TimeOut = false;
	unsigned long long NumTransmitedPackets = 0;
	unsigned long long NumTransmittedSamples = 0;
	double Delta = 0;
};

#endif /* STREAMER_H_ */
