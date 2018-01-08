/*
 * FileHandler.h
 *
 *  Created on: Dec 28, 2017
 *      Author: x300
 */

#ifndef FILEHANDLER_H_
#define FILEHANDLER_H_
#include "defs.h"
#include "Buffers.h"
#include "StreamingRequests.h"
#include <sys/stat.h>

class FileHandler {
	Buffers *pBuffers;
	StreamingRequests *pStreamReq;
	bool HasHeader = false;
	double Rate, Freq, Gain;
	unsigned long long CollectedSamples = 0;
	//unsigned long long NumSamplesFile;
	int BatchSize;
	bool EndFile = false;
	long long CollectedInFile;
	FILE *infile ;

public:
	FileHandler();
	virtual ~FileHandler();
	int Record2File( const char *FileName, int long long total_num_samps);
	void SetStreamer(Buffers *pBuffersX,  StreamingRequests *pStreamReqX);
	void SetParams(double RateIn, double FreqIn, double GainIn)
	{
		Rate = RateIn;
		Gain = GainIn;
		Freq = FreqIn;
	}
	int FirstRead(const char *FileName, int LoopMode1);
	int ContinueRead(const char *FileName, int LoopMode1);
};

#endif /* FILEHANDLER_H_ */
