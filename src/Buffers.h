/*
 * Buffers.h
 *
 *  Created on: Oct 31, 2017
 *      Author: x300
 */

#ifndef BUFFERS_H_
#define BUFFERS_H_
#include "defs.h"
using namespace std;
#include <complex>
#include <vector>
#define NUM_BUFFERS 8192



class Buffers {
	int PtrRd = 0;
	int PtrWr = 0;
	unsigned long long Overflows = 0;

	complex<short> *BigBuffer;

	std::complex<short> *Buffers1[NUM_BUFFERS];
public:
	int MinAvailable = NUM_BUFFERS;
	int NumBuffers = NUM_BUFFERS;
	Buffers(int samps_per_buff);
	virtual ~Buffers();
	void Reset(void) {
		PtrRd = 0;
		PtrWr = 0;
		Overflows = 0;
		NumBuffers = NUM_BUFFERS;
	}

	std::complex<short> *GetReadBuffer(void) {

		if((PtrRd == PtrWr) and (NumBuffers > 0))
		{
			return 0;
		}
		return Buffers1[PtrRd];
	}

	std::complex<short> *GetWriteBuffer(void)
	{
		if (NumBuffers == 0) {
			{
				Overflows++;
				cout<<"Buffer Ovf"<<endl;
				NumBuffers = NUM_BUFFERS;
				exit(-1);
			}

		}

		if(NumBuffers < MinAvailable)
		{
			MinAvailable = NumBuffers;
	//		cout <<NumBuffers<<endl;
		}

		NumBuffers--;
		return Buffers1[PtrWr];
	}



	std::complex<short> *GetMultipleWriteBuffer()
	{
			return Buffers1[PtrWr];
	}





	void ReleaseReadBuffer(int AddNumBuffers)
	{
		NumBuffers += AddNumBuffers;
	}


	void AdvanceReadBuffer(void) {
		int NewPtr = PtrRd;
		NewPtr++;
		NewPtr &= (NUM_BUFFERS - 1);

		PtrRd = NewPtr;
	//	cout<<"RD Wr Rd "<<PtrWr<<" "<<PtrRd<<endl;
	}

	void ReleaseWriteBuffer(void) {
		int NewPtr = PtrWr;
		NewPtr++;
		NewPtr &= (NUM_BUFFERS - 1);
		PtrWr = NewPtr;
	//	cout<<"WR Wr Rd "<<PtrWr<<" "<<PtrRd<<endl;

	}

	void AdvanceWriteBuffer(int Advance)
	{
		NumBuffers -= Advance;
		int NewPtr = PtrWr;
		NewPtr+= Advance;
		NewPtr &= (NUM_BUFFERS - 1);
		PtrWr = NewPtr;
	}

};

#endif /* BUFFERS_H_ */
