/*
 * Streamer.cpp
 *
 *  Created on: Oct 31, 2017
 *      Author: x300
 */


#include "Streamer.h"

extern bool stop_signal_called;

Streamer::Streamer(struct StreamerParams *Params) {
	// TODO Auto-generated constructor stub
	Buff1 = (Buffers *) Params->Buffer;
	usrp = Params->usrp;
	ReqQue = (StreamingRequests *) Params->RequestsQue;
	MaxTime = 0;
	num_requested_samples = 0;
	Rate = 0;
}

Streamer::~Streamer() {
	// TODO Auto-generated destructor stub
}

void Streamer::Run(void) {
	//create a receive streamer
	uhd::stream_args_t stream_args("sc16", "sc16");
	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);


	uhd::rx_metadata_t md;
	bool overflow_message = true;

	//setup streaming
	uhd::stream_cmd_t stream_cmd(
			uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
	//bool continue_on_bad_packet = false;

//	unsigned int LLog = 1 << 20;
//	unsigned int MaskLog = LLog - 1;
//	unsigned int PtrLog = 0;
//	double *LogTime = new double[LLog];

	cout
			<< "//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////"
			<< endl;
	while (not stop_signal_called) {

		num_requested_samples = -1;

		while (not stop_signal_called) {
			num_requested_samples = ReqQue->GetRequest(); //Get a new streaming request
			if (num_requested_samples != -1) {
				break;
			}
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));

		}

		if (stop_signal_called) {
			break;
		}

		int long long num_total_samps = 0;

		if ((num_requested_samples == 0)
				or (num_requested_samples >= 10000000)) {
			stream_cmd.stream_mode =
					uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
			stream_cmd.num_samps = size_t(0);

		} else {
			stream_cmd.stream_mode =
					uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE;
			stream_cmd.num_samps = size_t(num_requested_samples);

		}

		stream_cmd.stream_now = true;
		stream_cmd.time_spec = uhd::time_spec_t();
		rx_stream->issue_stream_cmd(stream_cmd);

		boost::this_thread::sleep(boost::posix_time::milliseconds(10));
	//	int BatchSize = SAMPS_PER_BUFF;

		int NumBuffers2Write = 128;

		while (not stop_signal_called
				and (num_requested_samples > num_total_samps)) {
/*
			struct timespec tp0, tp1;
			clock_gettime(CLOCK_MONOTONIC, &tp0);

			LogTime[PtrLog++] = tp0.tv_sec;
			LogTime[PtrLog++] = tp0.tv_nsec;
			PtrLog &= MaskLog;
*/
	//		int n = 0;

			complex<short> *NewBuffer = Buff1->GetMultipleWriteBuffer(NumBuffers2Write);
			size_t num_rx_samps;
			size_t n = 0;

			for(int ii = 0 ; ii < (NumBuffers2Write*NUM_BATCHES); ii++)
			{

				num_rx_samps = rx_stream->recv(NewBuffer + n, ONE_BATCH ,md, 3.0, false);
				n += num_rx_samps;

				if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
						TimeOut = true;
						std::cout << boost::format("Timeout while streaming")
								<< std::endl;
						break;
					}

					if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
						if (overflow_message) {
							Overflow = true;
							cout << "Drop/Overflow Error" << endl;
	/*
							FILE *fid = fopen("LogOut.dat", "wb");
							fwrite(LogTime, 8, LLog, fid);
							cout << " PtrLog " << PtrLog << endl;
							fclose(fid);
							fprintf(stderr,"EROR Rx Drop/Overflow Error\n");

							*/

	//					boost::this_thread::sleep(boost::posix_time::microseconds(1000));

							printf("EROR Drop\n");

						}
					}
			}
			/*	if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
					std::string error = str(
							boost::format("Receiver error: %s")
									% md.strerror());
					if (continue_on_bad_packet) {
						std::cerr << error << std::endl;
						continue;
					} else
						throw std::runtime_error(error);
				}
*/

	/*

	 		clock_gettime(CLOCK_MONOTONIC, &tp1);
			//					 double newtime = diff_time(tp0,tp1);
			LogTime[PtrLog++] = tp1.tv_sec;
			LogTime[PtrLog++] = tp1.tv_nsec;
			PtrLog &= MaskLog;
*/
			Buff1->AdvanceWriteBuffer(NumBuffers2Write);

			num_total_samps += n;
	//		cout<<" Num Samples "<< num_total_samps<<" "<<num_requested_samples<<endl;


//				 if(newtime > MaxTime)
//				 {
//					 MaxTime = newtime;
//					 cout<< "Max Time Stream "<<MaxTime <<endl;
//				 }

		}

		if (stream_cmd.stream_mode
				== uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS) {
			stream_cmd.stream_mode =
					uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
			rx_stream->issue_stream_cmd(stream_cmd);
		}

	}

	cout<<"Exiting Streamer"<<endl;
	/*
	FILE *fid = fopen("LogOut.dat", "wb");
	fwrite(LogTime, 8, LLog, fid);
	cout << " PtrLog " << PtrLog << endl;
	fclose(fid);
	*/

}
