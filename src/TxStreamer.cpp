/*
 * TxStreamer.cpp
 *
 *  Created on: Nov 6, 2017
 *      Author: x300
 */

#include "TxStreamer.h"
extern bool stop_signal_called;
extern unsigned long long NSamp;
extern unsigned long long NPack;

TxStreamer::TxStreamer(struct StreamerParams *Params) :
		Streamer(Params) {
	// TODO Auto-generated constructor stub
	ZeroBuffer = (short*) aligned_alloc(32,
			SAMPS_PER_BUFF * sizeof(complex<short> ));
	memset((void*) (ZeroBuffer), 0, SAMPS_PER_BUFF << 2);
}

TxStreamer::~TxStreamer() {
	// TODO Auto-generated destructor stub
	free(ZeroBuffer);
}

void TxStreamer::Run() {
	uhd::stream_args_t stream_args("sc16", "sc16");
	uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

	uhd::tx_metadata_t md;
	md.start_of_burst = false;
	md.end_of_burst = false;
	NumTransmittedSamples = 0;

	unsigned int LowRateBatch = SAMPS_PER_BUFF >>3;

	/*unsigned int LLog = 4096;
	 unsigned int MaskLog = LLog - 1;
	 unsigned int PtrLog = 0;
	 double *LogTime = new double [LLog];
	 */
	struct timespec tp0, tp1;

	double PacketRate = Rate / double(SAMPS_PER_BUFF);
	double InvPacketRate = 1.0 / PacketRate;
	clock_gettime(CLOCK_MONOTONIC, &tp0);

	cout << "Started Streaming " << endl;
	cout << "Time " << tp0.tv_sec << " " << tp0.tv_nsec << endl;
	while (not md.end_of_burst) {

		complex<short> *NewBuffer = Buff1->GetReadBuffer();
		if (NewBuffer == 0) {

			stop_signal_called = true;
		}

		if (stop_signal_called) {
			md.end_of_burst = true;
		}

		if (stop_signal_called) {
			tx_stream->send(ZeroBuffer, SAMPS_PER_BUFF, md);
		} else {
		//	clock_gettime(CLOCK_MONOTONIC, &tp0);
			/*
			 LogTime[PtrLog++] = tp0.tv_sec;
			 LogTime[PtrLog++] = tp0.tv_nsec;
			 PtrLog &= MaskLog;
			 */

			int n = 0;
//			if(Rate > MIN_TX_RATE)
//			{
//				n = tx_stream->send(NewBuffer, SAMPS_PER_BUFF, md);
//
//
//			}
//			else
//			{

				for(int kk = 0; kk < 8; kk++)
				{
					int k = tx_stream->send(NewBuffer + n , LowRateBatch, md);
					NSamp += k;
					n += k;
				}

	//		}

			/*			 LogTime[PtrLog++] = tp1.tv_sec;
			 LogTime[PtrLog++] = tp1.tv_nsec;
			 PtrLog &= MaskLog;

			 if(PtrLog == 0)
			 {

			 FILE *fid = fopen("LogOut.dat","wb");
			 fwrite(LogTime,8,LLog,fid);
			 fclose(fid);
			 cout<<"Time "<<tp1.tv_sec<<" "<<tp1.tv_nsec<<endl;
			 exit(-1);
			 }

			 */

			if (n != SAMPS_PER_BUFF) {
				cout << "Tx Error" << endl;
			}
		}
		Buff1->ReleaseReadBuffer(1);
		Buff1->AdvanceReadBuffer();
		NumTransmittedSamples += SAMPS_PER_BUFF;
		NumTransmitedPackets++;
		NPack = NumTransmitedPackets;
	/*
		if ((Rate < MIN_TX_RATE) && (NumTransmitedPackets >= 10)) {
			//Flow Control
			if ((NumTransmitedPackets & 1) == 0) {
	//			cout<<"Start"<<endl;

				while (1) {
					clock_gettime(CLOCK_MONOTONIC, &tp1);
	//						cout << "Time " << tp1.tv_sec << " " << tp1.tv_nsec << endl;

					double newtime = diff_time(tp0, tp1);
					double EstTxPackets = newtime * PacketRate;
					Delta = (double(NumTransmitedPackets)-EstTxPackets);
					if((Delta > 3.5) || (Delta < .5))
					{
						cout<<"Delta "<<Delta<<endl;
					}
					if (Delta > 1.25) {
						int SleepTime = int(0.1 * Delta * InvPacketRate * 1e6);
						//cout<<"Sleeping "<<SleepTime<<endl;
						boost::this_thread::sleep_for(
								boost::chrono::microseconds(SleepTime));

					} else {
						break;
					}
				}
	//			cout<<"End"<<endl;
			}

		}
*/
		if ((NumTransmittedSamples & 0xFFFFFF) == 0) {
			cout << "Played " << NumTransmittedSamples << endl;
			cout << "Time " << tp1.tv_sec << " " << tp1.tv_nsec << endl;
		}
	}

	cout << "Transmitted " << NumTransmittedSamples << endl;

}
