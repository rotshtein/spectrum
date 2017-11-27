/*
 * SpectrumCalc.h
 *
 *  Created on: Oct 26, 2017
 *      Author: x300
 */

#ifndef SPECTRUMCALC_H_
#define SPECTRUMCALC_H_
#include "defs.h"
#include <fftw3.h>
#include <math.h>
#include <immintrin.h>
#define RE1 0
#define IM1 1

#include "Buffers.h"
#include "StreamingRequests.h"
 #include <time.h>

#include <iostream>
using namespace std;


class SpectrumCalc {

	Buffers *pBuffers;
	StreamingRequests *pStreamReq;

	int LengthComplex = MFFT*NFFT;
	fftwf_complex *in, *out, *buffer;
	fftwf_plan p;
	fftwf_complex *hamm;
	const int N = NFFT;
	int PtrBuff = 0;
	void CalcHammingWindow(void);
	void CreateOne(fftwf_complex *In, fftwf_complex *Out);
	fftwf_complex *SumSignal;
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

	SpectrumCalc();
	virtual ~SpectrumCalc();
	float Convert(short *In, int Length);
	float CalcPower(short *In, int Length);
	int CreateOutput(const char *FileName, float CenterFreq, float Rate);
	int Record2File(const char *FileName, int long long total_num_samps);
	void SetStreamer(Buffers *pBuffersX,  StreamingRequests *pStreamReqX);
};

#endif /* SPECTRUMCALC_H_ */
