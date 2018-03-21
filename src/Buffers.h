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
#include <atomic>
#ifdef SERVER_VERSION
	#define NUM_BUFFERS 131072
#else
	#define NUM_BUFFERS 32768
#endif
//#include <pthread.h>


class Buffers {
	unsigned long long Overflows = 0;
	bool Lock = false;
	complex<short> *BigBuffer;
	//pthread_mutex_t lock;

	std::complex<short> *Buffers1[NUM_BUFFERS];
public:
	int PtrRd = 0;
	int PtrWr = 0;
	int MinAvailable = NUM_BUFFERS;
	std::atomic<int> NumBuffers;
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

//		if(NumBuffers >=  (NUM_BUFFERS))
//		{
//			cout<<"Read Error "<<NumBuffers<<" "<<PtrWr<<" "<<PtrRd<<endl;
//			exit(-1);
//		}

		return Buffers1[PtrRd];
	}


	std::complex<short> *GetReadBuffer(int N) {

			if((NumBuffers > (NUM_BUFFERS-(N<<1))))//Guaranteed that there is info
			{
				return 0;
			}



			return Buffers1[PtrRd];
		}


	void ReleaseReadBuffer(int AddNumBuffers)
	{
			Lock = true;
	//		 pthread_mutex_lock(&lock);
	//		 NumBuffers += AddNumBuffers;
	//		 pthread_mutex_unlock(&lock);
			 /*int Diff = PtrWr-PtrRd;
				if(Diff < 0)
					Diff += NUM_BUFFERS;
			*/
			NumBuffers +=AddNumBuffers;


		//	cout<<"RD "<<NumBuffers<<" "<<PtrWr<<" "<<PtrRd<<" "<<Diff<<endl;


			Lock = false;

	}


	void AdvanceReadBuffer(int N) {
				int NewPtr = PtrRd;
				NewPtr+=N;
				NewPtr &= (NUM_BUFFERS - 1);

				PtrRd = NewPtr;
			//	cout<<"RD Wr Rd "<<PtrWr<<" "<<PtrRd<<endl;
			}



		void AdvanceReadBuffer(void) {
			int NewPtr = PtrRd;
			NewPtr++;
			NewPtr &= (NUM_BUFFERS - 1);

			PtrRd = NewPtr;
		//	cout<<"RD Wr Rd "<<PtrWr<<" "<<PtrRd<<endl;
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

//		 pthread_mutex_lock(&lock);
//		 NumBuffers--;
//		 pthread_mutex_unlock(&lock);

		NumBuffers--;

		return Buffers1[PtrWr];
	}



	std::complex<short> *GetMultipleWriteBuffer(int N)
	{
		if(NumBuffers < N)
		{
			Overflows++;
			cout<<"Buffer Ovf"<<endl;
			NumBuffers = NUM_BUFFERS;
			exit(-1);
		}
//		cout<<"Write "<<NumBuffers<<endl;
//		pthread_mutex_lock(&lock);
//		NumBuffers -= N;
//		pthread_mutex_unlock(&lock);

		NumBuffers -=N;

//		if(Lock)
//		{
//			cout<<"Write "<<NumBuffers<<endl;
//		}
		return Buffers1[PtrWr];
	}

	void AdvanceWriteBuffer(int Advance)
		{
			int NewPtr = PtrWr;
			NewPtr+= Advance;
			NewPtr &= (NUM_BUFFERS - 1);
			PtrWr = NewPtr;
		}




	void ReleaseWriteBuffer(void) {
		int NewPtr = PtrWr;
		NewPtr++;
		NewPtr &= (NUM_BUFFERS - 1);
		PtrWr = NewPtr;
	//	cout<<"WR Wr Rd "<<PtrWr<<" "<<PtrRd<<endl;

	}

	void ReleaseMultipleWriteBuffer(int N) {
		int NewPtr = PtrWr;
		NewPtr += N;
		NewPtr &= (NUM_BUFFERS - 1);
		PtrWr = NewPtr;
	//	cout<<"WR Wr Rd "<<PtrWr<<" "<<PtrRd<<endl;

	}



};

#endif /* BUFFERS_H_ */
