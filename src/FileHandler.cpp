/*
 * FileHandler.cpp
 *
 *  Created on: Dec 28, 2017
 *      Author: x300
 */

#include "FileHandler.h"
extern bool stop_signal_called;
extern long long NumSamplesFile;
static char Header[8] ={'R','E','C','O','r','d','e','r'};
extern FILE *StatFile;
extern unsigned long long Time2PrintMask;
extern unsigned long MaxSamplesTx;
FileHandler::FileHandler() {
	Gain = 0;
	Freq = 0;
	Rate = 0;
	pStreamReq = NULL;
	pBuffers = NULL;
}

FileHandler::~FileHandler() {
	// TODO Auto-generated destructor stub
}

void FileHandler::SetStreamer(Buffers *pBuffersX,  StreamingRequests *pStreamReqX)
{
	pBuffers = pBuffersX;
	pStreamReq = pStreamReqX;
}


int FileHandler::Record2File( const char *FileName, int long long total_num_samps) {

	int NumAvailableBuffers = 0;
	//double MaxTime = 0;
//	  std::ofstream outfile;
//	  outfile.open(FileName, std::ofstream::binary);

	int NumBuffers2Write = 4096;
	pBuffers->Reset();
	FILE *outfile;
	outfile = fopen(FileName,"wb");
	double version = VERSION;

	//Write Header
	fwrite( Header,1,8,outfile);
	fwrite( &version,1,8,outfile);
	fwrite( &Rate,1,8,outfile);
	fwrite( &Freq,1,8,outfile);
	fwrite( &Gain,1,8,outfile);


	int long long num_samples_collected = 0;

	total_num_samps += SAMPS_PER_BUFF;

	pStreamReq->InsertRequest(total_num_samps);
	//bool First = true;
	double SumTime = 0.0;
	complex <short> *BaseBuffer = 0;

	unsigned long long LastPrintSamples = 0;
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
					fprintf(StatFile,"EROR DISK\n");
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
		if((num_samples_collected - LastPrintSamples) >=  Time2PrintMask)
		{
			LastPrintSamples = num_samples_collected;
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

int FileHandler::FirstRead(const char *FileName, int LoopMode1)
{
	bool LoopMode = true;
	if(LoopMode1 == 0)
	{
		LoopMode = false;
	}
	cout << "Playing from file" << endl;
	struct stat file_stats;
	stat(FileName, &file_stats);

	long long FileSize = file_stats.st_size;
		NumSamplesFile = FileSize >> 2;
		BatchSize = BatchSizeTx;

		long long NumBatches = NumSamplesFile / BatchSize;
		if ((NumBatches * BatchSize) < NumSamplesFile) {
			NumBatches++;
		}
		NumSamplesFile = NumBatches * BatchSize;
		infile = fopen(FileName, "rb");
		char Header1[8];

		fread(Header1,1,8,infile);

		HasHeader = true;


		for(int ii = 0;ii < 8; ii++)
		{
			if(Header1[ii] != Header[ii])
			{
				HasHeader = false;
				break;
			}
		}

		fseek(infile, 0, SEEK_SET);

		if(HasHeader)
		{
			cout<<" Has Header "<<endl;
			fseek(infile, HEADER_LENGTH , SEEK_SET);
		}


		while (pBuffers->NumBuffers > 0) {
			complex<short> *NewBuffer = pBuffers->GetMultipleWriteBuffer(32);
			int n = fread(NewBuffer, 4, BatchSize, infile);

			CollectedSamples += n;
			if (n < BatchSize) {
				EndFile = true;
				memset((void*) (NewBuffer + n), 0, (BatchSize - n) << 2);
				pBuffers->AdvanceWriteBuffer(32);

				if (LoopMode) {

					fseek(infile, 0, SEEK_SET);
					if(HasHeader)
							{
								fseek(infile, HEADER_LENGTH , SEEK_SET);
							}

					EndFile = false;
				} else {
					break;
				}

			}

			pBuffers->AdvanceWriteBuffer(32);
		}


		CollectedInFile = 0;

		cout << "Read file again" << endl;
			if (EndFile and not LoopMode) {
				fclose(infile);
			} else {
				if (EndFile and LoopMode) {
					fseek(infile, 0, SEEK_SET);
					if(HasHeader)
							{
								fseek(infile, HEADER_LENGTH , SEEK_SET);
							}

				}
			}
	return 0;
}


int FileHandler::ContinueRead(const char *FileName, int LoopMode1)
{
	bool LoopMode = true;
		if(LoopMode1 == 0)
		{
			LoopMode = false;
		}
	bool RdTmp = false;
	int NumBuffers;
	while (not stop_signal_called) {
		while (1) {
			NumBuffers = pBuffers->NumBuffers;

			if (NumBuffers >= 32) {
				break;
			}

			boost::this_thread::sleep(
					boost::posix_time::milliseconds(1));

		}
	//	cout<<"Num Buffers "<<NumBuffers<<endl;
		complex<short> *NewBuffer = pBuffers->GetMultipleWriteBuffer(32);
	//	cout<<objBuffers.PtrRd<<" "<<objBuffers.PtrWr<<endl;
		RdTmp = true;
		int n = fread(NewBuffer, 4, BatchSize, infile);
		CollectedSamples += n;
		CollectedInFile += n;

//			cout<<"Read "<<CollectedInFile<<endl;
		if ((CollectedInFile & Time2PrintMask) == 0)
			cout << "Read Samples" << CollectedSamples << endl;
		if (n < BatchSize) {
			EndFile = true;
			if (n > 0) {
				memset((void*) (NewBuffer + n), 0,
						(BatchSize - n) << 2);
		//		objBuffers.AdvanceWriteBuffer(32);
				CollectedSamples += (BatchSize - n);
			}
			else
			{
				RdTmp = false;
			}

			if (LoopMode) {
				fseek(infile, 0, SEEK_SET);
				if(HasHeader)
						{
							fseek(infile, HEADER_LENGTH , SEEK_SET);
						}
				CollectedInFile = 0;
			} else {
				break;
			}

		}
		if(RdTmp)
		{
			RdTmp = false;
			pBuffers->AdvanceWriteBuffer(32);
		}

	}
	if (LoopMode) {
		fclose(infile);
	}
	return 0;
}

void FileHandler::SetBatchSize()
{
	int n = (SAMPS_PER_BUFF/MaxSamplesTx);
	BatchSizeTx = n * MaxSamplesTx * 32;


}


