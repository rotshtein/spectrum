/*
 * SpectrumCalc.cpp
 *
 *  Created on: Oct 26, 2017
 *      Author: x300
 */

#include "SpectrumCalc.h"
extern bool stop_signal_called;
extern FILE *StatFile;
SpectrumCalc::SpectrumCalc() {
	// TODO Auto-generated constructor stub
	hamm = (fftwf_complex*) fftw_malloc(sizeof(fftwf_complex) * N);
	CalcHammingWindow();
	out = (fftwf_complex*) fftw_malloc(sizeof(fftwf_complex) * N);
	in = (fftwf_complex*) fftw_malloc(sizeof(fftwf_complex) * N);
	buffer = (fftwf_complex*) fftw_malloc(sizeof(fftwf_complex) * LengthComplex);
	SumSignal = (fftwf_complex*) fftw_malloc(sizeof(fftwf_complex) * 4);

	for(int ii = 0;ii<4;ii++)
	{
		SumSignal[ii][RE1]=0.0;
		SumSignal[ii][IM1]=0.0;

	}

//	cout << (double)boost::posix_time::time_duration::ticks_per_second()<<endl;


}

SpectrumCalc::~SpectrumCalc() {
	// TODO Auto-generated destructor stub
	fftw_free(hamm);
	fftw_free(out);
	fftw_free(in);
	fftwf_destroy_plan(p);
	fftw_free(buffer);
	fftw_free(SumSignal);

}


void SpectrumCalc::SetStreamer(Buffers *pBuffersX,  StreamingRequests *pStreamReqX)
{
	pBuffers = pBuffersX;
	pStreamReq = pStreamReqX;
}

void SpectrumCalc::CalcHammingWindow() {
	float pi = 4.0 * atan(1.0);
	float k = 2.0 * pi / float(N - 1);

	float a = 0.54;
	float b = 1 - a;

	for (int ii = 0; ii < N; ii++) {
		float h = a - b * cos(k * float(ii));
		hamm[ii][RE1] = h;
		hamm[ii][IM1] = h;

	}
}

float SpectrumCalc::Convert(short *In, int Length) {

	float *Out1 = (float *) (buffer + PtrBuff) ;
	__m256 mSum = _mm256_loadu_ps((float*) SumSignal);

	int Length2 = Length << 1;

	int NextPtr = PtrBuff + Length;
	if(NextPtr > LengthComplex) //Longer than the Buffer
	{
		return 0;
	}

	__m256 mK = _mm256_set1_ps(0.0009765625);

	int PtrOut = 0;

	for (int ii = 0; ii < Length2; ii += 16) {

		__m256i mIn = _mm256_loadu_si256((__m256i *) (In + ii));
		__m256i mInhlf = _mm256_cvtepi16_epi32(
				_mm256_extracti128_si256(mIn, 0));
		__m256 x = _mm256_cvtepi32_ps(mInhlf);
		x = _mm256_mul_ps(x, mK);
		mInhlf = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(mIn, 1));
		__m256 y = _mm256_cvtepi32_ps(mInhlf);
		y = _mm256_mul_ps(y, mK);
		mSum = _mm256_add_ps(x,mSum);
		_mm256_storeu_ps(Out1 + PtrOut, x);
		PtrOut += 8;
		mSum = _mm256_add_ps(y,mSum);
		_mm256_storeu_ps(Out1 + PtrOut, y);
		PtrOut += 8;

	}

	PtrBuff = NextPtr;
	_mm256_storeu_ps((float*)(SumSignal), mSum);
	return 0;
}




float SpectrumCalc::CalcPower(short *In, int Length) {


	int Length2 = Length << 1;




	__m256 mSum = _mm256_setzero_ps();

	for (int ii = 0; ii < Length2; ii += 16) {

		__m256i mIn = _mm256_loadu_si256((__m256i *) (In + ii));
		__m256i mIn2 = _mm256_madd_epi16(mIn, mIn);//^2
		__m256 y = _mm256_cvtepi32_ps(mIn2);
		mSum = _mm256_add_ps(y,mSum);

	}


	 float array[8] __attribute__ ((aligned(32)));

	 _mm256_store_ps(array,mSum);

	 float Sum = 0;

	 for(int ii = 0; ii < 8; ii++)
	 {
		 Sum += array[ii];
	 }



	return (Sum/float(Length2));
}

int SpectrumCalc::Record2File( const char *FileName, int long long total_num_samps) {

	int NumAvailableBuffers = 0;
	//double MaxTime = 0;
//	  std::ofstream outfile;
//	  outfile.open(FileName, std::ofstream::binary);

	int NumBuffers2Write = 4096;
	pBuffers->Reset();
	FILE *outfile;
	outfile = fopen(FileName,"wb");
	int long long num_samples_collected = 0;

	total_num_samps += SAMPS_PER_BUFF;

	pStreamReq->InsertRequest(total_num_samps);
	//bool First = true;
	double SumTime = 0.0;
	complex <short> *BaseBuffer = 0;
	while((num_samples_collected <  total_num_samps) and (not stop_signal_called))
	{
		//Stream SAMPS_PER_BUFF

		complex<short> *NewBuffer = 0;
		while(1)
		{
			NewBuffer = pBuffers->GetReadBuffer();
			if(NewBuffer != 0)
			{
				break;
			}

			boost::this_thread::sleep(boost::posix_time::microseconds(1));
		}

		{
			if(NumAvailableBuffers == 0)
			{
				BaseBuffer = NewBuffer;
			}

			pBuffers->AdvanceReadBuffer();

			NumAvailableBuffers++;

			if(NumAvailableBuffers == NumBuffers2Write)
			{
	//			struct timespec tp0, tp1;
	//			clock_gettime(CLOCK_MONOTONIC, &tp0);


				int m = fwrite((const char*) BaseBuffer,1,SAMPS_PER_BUFF*4*NumBuffers2Write,outfile);
				if(m == 0)
				{
					fprintf(StatFile,"%s", "EROR DISK\n");
					fflush(StatFile);
					stop_signal_called = true;
				}

		//		 clock_gettime(CLOCK_MONOTONIC, &tp1);
	//			 double newtime = diff_time(tp0,tp1);
		//		 SumTime += newtime;
			//	 if(newtime > MaxTime)
				// {
					// MaxTime = newtime;
					 //cout<< MaxTime <<endl;
				// }


				 pBuffers->ReleaseReadBuffer(NumBuffers2Write);
				 NumAvailableBuffers = 0;
			}


		}

		num_samples_collected += SAMPS_PER_BUFF;
		if((num_samples_collected & 0x7FFFFFFF) == 0)
		{
			cout <<"Collected "<<num_samples_collected<<" Buffers "<< pBuffers->NumBuffers<<endl;
			fprintf(StatFile,"RCRD Samples: %lld of: %lld\n",num_samples_collected,total_num_samps);
			fflush(StatFile);


		}
	}

	//outfile.close();
	fclose(outfile);
	cout<<"Time Elapsed "<<SumTime<<endl;
	return 0;
}

int SpectrumCalc::CreateOutput( const char *FileName, float Rate, float CenterFreq) {


	fftwf_complex *Spectrum2 = (fftwf_complex*) fftw_malloc(
			sizeof(fftwf_complex) * N);

	if (Spectrum2 == 0) {
		return -1;
	}

	//Zeroize

	__m256 mZero = _mm256_setzero_ps();

	for (int ii = 0; ii < (N << 1); ii += 8) {
		_mm256_storeu_ps((float *) Spectrum2 + ii, mZero);
	}



	p = fftwf_plan_dft_1d(N, buffer, out, FFTW_FORWARD, FFTW_ESTIMATE);


	//Collect Samples

	int total_num_samps = NFFT*(MFFT+2);

	pStreamReq->InsertRequest(total_num_samps);

	bool First = true;

	long long num_samples_collected = 0;

	while((num_samples_collected <  total_num_samps) and (not stop_signal_called))
	{
		//Stream SAMPS_PER_BUFF

		complex<short> *NewBuffer = 0;
		while(1)
		{
			NewBuffer = pBuffers->GetReadBuffer();
			if(NewBuffer != 0)
			{
				break;
			}

			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
		}
		if(First)//Throw first block
		{
			First = false;
		}
		else
		{
			Convert((short *) NewBuffer, SAMPS_PER_BUFF);
		}
		pBuffers->AdvanceReadBuffer();
		pBuffers->ReleaseReadBuffer(1);
		num_samples_collected += SAMPS_PER_BUFF;
	}

	if(stop_signal_called)
	{
		return -1;
	}


	int LengthAll = PtrBuff;

	cout<<"Collected "<<LengthAll<<"Samples"<<endl;

	float Si = 0.0;
	float Sq = 0.0;
	for(int ii = 0; ii < 4;ii++)
	{
		Si += SumSignal[ii][RE1];
		Sq += SumSignal[ii][IM1];

	}

	float k1 = 1.0/float(LengthAll);
	Si *= k1;
	Sq *= k1;

	//Remove DC

	for (int ii = 0; ii < 4; ii++) {
		SumSignal[ii][RE1] = Si;
		SumSignal[ii][IM1] = Sq;

	}

	__m256 mDC=  _mm256_load_ps((float *) SumSignal);

	for (int ii = 0; ii < (LengthAll<<1); ii+=8) {
		__m256 mIn = _mm256_load_ps((float *) buffer + ii);
		mIn = _mm256_sub_ps(mIn, mDC);
		_mm256_store_ps((float *) buffer + ii,mIn);
	}

	int NumFFTs = 2 * LengthAll / N - 1;
	int PtrIn = 0;
	int N2 = N << 1;

	__m256 mK = _mm256_set1_ps(1.0 / float(NumFFTs));

	for (int ii = 0; ii < NumFFTs; ii++) {

		CreateOne(buffer + PtrIn, out);
		PtrIn += (N >> 1);

		//add new fft to spectrum

		for (int jj = 0; jj < N2; jj += 8) {
			__m256 mIn = _mm256_loadu_ps((float *) Spectrum2 + jj);
			__m256 mNew = _mm256_loadu_ps((float *) out + jj);
			mNew = _mm256_mul_ps(mNew, mNew); // ^2
			mNew = _mm256_mul_ps(mNew, mK); //Normalize and add
			mIn = _mm256_add_ps(mNew, mIn);
			_mm256_storeu_ps((float *) Spectrum2 + jj, mIn);
		}

	}

	//Finalize spectrum

	__m256i mIdx = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0);

	int PtrOut = 0;

	for (int ii = 0; ii < N2; ii += 16) {
		__m256 mIna = _mm256_loadu_ps((float *) Spectrum2 + ii);
		__m256 mInb = _mm256_loadu_ps((float *) Spectrum2 + ii + 8);
		__m256 mOut = _mm256_hadd_ps(mIna, mInb);
		mOut = _mm256_permutevar8x32_ps(mOut, mIdx);
		_mm256_storeu_ps((float *) Spectrum2 + PtrOut, mOut);
		PtrOut += 8;
	}

	float *Sp = (float *) Spectrum2;

	float *SpOut = (float *) aligned_alloc(32, NFFT * sizeof(float));
	if (SpOut == 0) {
			return -1;
		}
	float *SpOutAll = (float *) aligned_alloc(32, 2*NFFT * sizeof(float));
	if (SpOutAll == 0) {
			return -1;
		}

	PtrOut = N >> 1;

	for (int ii = 0; ii < N / 2; ii++) {
		SpOut[PtrOut++] = 10.0 * log10(Sp[ii]);
	}

	PtrOut = 0;

	for (int ii =  N / 2; ii < N; ii++) {
		SpOut[PtrOut++] = 10.0 * log10(Sp[ii]);
	}


	float Resolution = (1.0/float(NFFT)) * Rate;
	for (int ii =  0; ii < N; ii++) {

		SpOutAll[ii<<1] = float(ii-(N>>1))*Resolution+CenterFreq;
		SpOutAll[(ii<<1)+ 1] = SpOut[ii];
	}


	FILE *fid = fopen(FileName, "wb");
	if(fid == NULL)
		return -1;
	else
	{
		fwrite(SpOutAll,sizeof(float),N<<1,fid);
	}
	fclose(fid);

	fftw_free(Spectrum2);
	free(SpOut);

	return 1;

}

void SpectrumCalc::CreateOne(fftwf_complex *In, fftwf_complex *Out) {

	//Multiply by hamming window
	for (int ii = 0; ii < (N << 1); ii += 8) {
		__m256 mIn = _mm256_loadu_ps((float*) In + ii);
		__m256 mh = _mm256_loadu_ps((float*) hamm + ii);
		__m256 mx = _mm256_mul_ps(mIn, mh);
		_mm256_storeu_ps((float*) in + ii, mx);
	}

	fftwf_execute_dft(p, in, Out);

}
