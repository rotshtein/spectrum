//============================================================================
// Name        : Spectrum.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdio.h>
using namespace std;
#include <stdlib.h>
#include <math.h>
#include "defs.h"
#include "SpectrumCalc.h"
#include "Buffers.h"
#include "StreamingRequests.h"
#include "Streamer.h"
#include "TxStreamer.h"

unsigned long long NSamp, NPack;
Streamer *pobjSt;
namespace po = boost::program_options;

bool stop_signal_called = false;
void sig_int_handler(int) {
	stop_signal_called = true;
}


void my_handler(uhd::msg::type_t type, const std::string &msg){
//handle the message...
	cout<<msg<<endl;
	if(msg == "U")
	{
		struct timespec tp1;

		clock_gettime(CLOCK_MONOTONIC, &tp1);

		cout<<"NPack NSamp "<<NPack<<" "<<NSamp<<endl;
		cout << "Time " << tp1.tv_sec << " " << tp1.tv_nsec << endl;
		cout<<"Delta "<<pobjSt->Delta<<endl;
		exit(-1);
	}
	if(type == 'e')
	{
		exit(-1);
	}

}



SpectrumCalc objSpec;
static int Ctr = 0;



void displayAndChange(boost::thread& daThread)
{
    int retcode;
    int policy;

    pthread_t threadID = (pthread_t) daThread.native_handle();

    struct sched_param param;

    if ((retcode = pthread_getschedparam(threadID, &policy, &param)) != 0)
    {
        errno = retcode;
        perror("pthread_getschedparam");
        exit(EXIT_FAILURE);
    }

    std::cout << "INHERITED: ";
    std::cout << "policy=" << ((policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                               (policy == SCHED_RR)    ? "SCHED_RR" :
                               (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                                                         "???")
              << ", priority=" << param.sched_priority << std::endl;


    policy = SCHED_FIFO;
    param.sched_priority = 4;

    if ((retcode = pthread_setschedparam(threadID, policy, &param)) != 0)
    {
        errno = retcode;
        perror("pthread_setschedparam");
        exit(EXIT_FAILURE);
    }

    std::cout << "  CHANGED: ";
    std::cout << "policy=" << ((policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                               (policy == SCHED_RR)    ? "SCHED_RR" :
                               (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                                                          "???")
              << ", priority=" << param.sched_priority << std::endl;
}










template<typename samp_type> void recv_to_file(uhd::usrp::multi_usrp::sptr usrp,
		const std::string &cpu_format, const std::string &wire_format,
		const std::string &file, size_t samps_per_buff,
		unsigned long long num_requested_samples, double time_requested = 0.0,
		bool bw_summary = false, bool stats = false, bool null = false,
		bool enable_size_map = false, bool continue_on_bad_packet = false) {
	unsigned long long num_total_samps = 0;
	//create a receive streamer
	uhd::stream_args_t stream_args(cpu_format, wire_format);
	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

	uhd::rx_metadata_t md;
	std::vector<samp_type> buff(samps_per_buff);

	bool overflow_message = true;

	//setup streaming
	uhd::stream_cmd_t stream_cmd(
			uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
	stream_cmd.num_samps = size_t(num_requested_samples);
	stream_cmd.stream_now = true;
	stream_cmd.time_spec = uhd::time_spec_t();
	rx_stream->issue_stream_cmd(stream_cmd);

	boost::this_thread::sleep(boost::posix_time::milliseconds(10));

	cout << "Collecting Samples" << endl;

	while (not stop_signal_called and (num_requested_samples > num_total_samps)) {

		size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(), md,
				3.0, enable_size_map);

		if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
			std::cout << boost::format("Timeout while streaming") << std::endl;
			break;
		}
		if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
			if (overflow_message) {
				//  overflow_message = false;
				/*     std::cerr << boost::format(
				 "Got an overflow indication in collection. Please consider the following:\n"
				 "  Your write medium must sustain a rate of %fMB/s.\n"
				 "  Dropped samples will not be written to the file.\n"
				 "  Please modify this example for your purposes.\n"
				 "  This message will not appear again.\n"
				 ) % (usrp->get_rx_rate()*sizeof(samp_type)/1e6);*/
			}
			continue;
		}
		if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
			std::string error = str(
					boost::format("Receiver error: %s") % md.strerror());
			if (continue_on_bad_packet) {
				std::cerr << error << std::endl;
				continue;
			} else
				throw std::runtime_error(error);
		}

		if (Ctr == 0) {
			Ctr++; //Throw the first batch
		} else {
			Ctr++;
			objSpec.Convert((short *) &buff.front(), num_rx_samps);
			if ((Ctr & 0xf) == 0) {
				cout << num_total_samps << endl;
			}
		}

		num_total_samps += num_rx_samps;

	}

	stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
	rx_stream->issue_stream_cmd(stream_cmd);
	cout << "Finished Collecting" << endl;
}

template<typename samp_type> void agc(uhd::usrp::multi_usrp::sptr usrp,
		const std::string &cpu_format, const std::string &wire_format,
		const std::string &file, size_t samps_per_buff,
		unsigned long long num_requested_samples, double time_requested = 0.0,
		bool bw_summary = false, bool stats = false, bool null = false,
		bool enable_size_map = false, bool continue_on_bad_packet = false,
		double gain = 15) {

	//create a receive streamer
	uhd::stream_args_t stream_args(cpu_format, wire_format);
	uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

	uhd::rx_metadata_t md;
	std::vector<samp_type> buff(samps_per_buff);

	bool overflow_message = true;

	//setup streaming

	bool Converged = false;
	uhd::stream_cmd_t stream_cmd(
			uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
	stream_cmd.num_samps = size_t(samps_per_buff);
	stream_cmd.stream_now = true;
	stream_cmd.time_spec = uhd::time_spec_t();
	double gain1 = 0.0;
	while (!Converged) {

		stream_cmd.stream_mode =
				uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE;

		rx_stream->issue_stream_cmd(stream_cmd);

		boost::this_thread::sleep(boost::posix_time::milliseconds(100));

		{

			size_t num_rx_samps = rx_stream->recv(&buff.front(), buff.size(),
					md, 3.0, enable_size_map);

			if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_TIMEOUT) {
				std::cout << boost::format("Timeout while streaming")
						<< std::endl;
				break;
			}
			if (md.error_code == uhd::rx_metadata_t::ERROR_CODE_OVERFLOW) {
				if (overflow_message) {
					overflow_message = false;
					std::cerr
							<< boost::format(
									"Got an overflow indication in agc. Please consider the following:\n"
											"  Your write medium must sustain a rate of %fMB/s.\n"
											"  Dropped samples will not be written to the file.\n"
											"  Please modify this example for your purposes.\n"
											"  This message will not appear again.\n")
									% (usrp->get_rx_rate() * sizeof(samp_type)
											/ 1e6);
				}
				continue;
			}
			if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
				std::string error = str(
						boost::format("Receiver error: %s") % md.strerror());
				if (continue_on_bad_packet) {
					std::cerr << error << std::endl;
					continue;
				} else
					throw std::runtime_error(error);
			}

			{

				float Power = objSpec.CalcPower(
						((short *) &buff.front()) + 1000, num_rx_samps - 500);
				cout << "Gain, Power: " << gain << " " << Power << endl;
				float InvPower = 1.0f / Power;
				float Ratio = 4194304.0 * InvPower;
				float Delta = log10(Ratio);
				Delta *= 10.0f;

				float NewGain = gain + Delta;
				NewGain = floor(NewGain * 2 + 0.5) * 0.5;
				if (NewGain > 30.5) {
					NewGain = 30.5;
				} else if (NewGain < 0) {
					NewGain = 0;
				}

				if (abs(gain - NewGain) <= 0.5) {
					Converged = true;
				} else {
					gain = NewGain;
				}
			}

		}

		//	stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
		//	rx_stream->issue_stream_cmd(stream_cmd);
		if (!Converged) {
			usrp->set_rx_gain(gain);
			std::cout
					<< boost::format("Actual RX Gain: %f dB...")
							% usrp->get_rx_gain() << std::endl << std::endl;

		}

		boost::this_thread::sleep(boost::posix_time::milliseconds(100));

		gain1 = usrp->get_rx_gain();
		cout << "AGC Step, Gain =  " << gain1 << endl;
	}
}

void AGC2(uhd::usrp::multi_usrp::sptr usrp, double &gain, Buffers *pBuffers,
		StreamingRequests *pReq) {
	gain = 15.0;

	bool Converged = false;

	while ((not Converged) and (not stop_signal_called)) {
		//Stream SAMPS_PER_BUFF

		pReq->InsertRequest(SAMPS_PER_BUFF);
		complex<short> *NewBuffer = 0;
		while (1) {
			NewBuffer = pBuffers->GetReadBuffer();
			if (NewBuffer != 0) {
				break;
			}

			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
		}

		float Power = objSpec.CalcPower(((short *) NewBuffer) + 1000,
		SAMPS_PER_BUFF - 500);

		pBuffers->AdvanceReadBuffer();
		pBuffers->ReleaseReadBuffer(1);
		float InvPower = 1.0f / Power;
		float Ratio = 4194304.0 * InvPower;
		float Delta = log10(Ratio);
		Delta *= 10.0f;
		cout << "Current Gain, Power, Delta: " << gain << " " << Power << " "<<Delta<< endl;

		float NewGain = gain + Delta;
		NewGain = floor(NewGain * 2 + 0.5) * 0.5;
		if (NewGain > 30.5) {
			NewGain = 30.5;
		} else if (NewGain < 0) {
			NewGain = 0;
		}

		if (abs(gain - NewGain) <= 0.5) {
			Converged = true;
		} else {
			gain = NewGain;
		}

		//	rx_stream->issue_stream_cmd(stream_cmd);
		if (!Converged) {
			usrp->set_rx_gain(gain);
			std::cout
					<< boost::format("Actual RX Gain: %f dB...")
							% usrp->get_rx_gain() << std::endl << std::endl;
			gain = usrp->get_rx_gain();
			boost::this_thread::sleep(boost::posix_time::milliseconds(10));
			cout << "AGC Step, Gain =  " << gain << endl;
		}

	}
}

typedef boost::function<uhd::sensor_value_t(const std::string&)> get_sensor_fn_t;

bool check_locked_sensor(std::vector<std::string> sensor_names,
		const char* sensor_name, get_sensor_fn_t get_sensor_fn,
		double setup_time) {
	if (std::find(sensor_names.begin(), sensor_names.end(), sensor_name)
			== sensor_names.end())
		return false;

	boost::system_time start = boost::get_system_time();
	boost::system_time first_lock_time;

	std::cout << boost::format("Waiting for \"%s\": ") % sensor_name;
	std::cout.flush();

	while (true) {
		if ((not first_lock_time.is_not_a_date_time())
				and (boost::get_system_time()
						> (first_lock_time
								+ boost::posix_time::seconds(setup_time)))) {
			std::cout << " locked." << std::endl;
			break;
		}
		if (get_sensor_fn(sensor_name).to_bool()) {
			if (first_lock_time.is_not_a_date_time())
				first_lock_time = boost::get_system_time();
			std::cout << "+";
			std::cout.flush();
		} else {
			first_lock_time = boost::system_time();	//reset to 'not a date time'

			if (boost::get_system_time()
					> (start + boost::posix_time::seconds(setup_time))) {
				std::cout << std::endl;
				throw std::runtime_error(
						str(
								boost::format(
										"timed out waiting for consecutive locks on sensor \"%s\"")
										% sensor_name));
			}
			std::cout << "_";
			std::cout.flush();
		}
		boost::this_thread::sleep(boost::posix_time::milliseconds(100));
	}
	std::cout << std::endl;
	return true;
}

void RunStreamer(void *PtrIn) {
	Streamer *Ptr = (Streamer *) PtrIn;
	Ptr->Run();

}

int UHD_SAFE_MAIN(int argc, char *argv[]){
//cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!

	uhd::msg::register_handler(&my_handler);
	uhd::set_thread_priority_safe();

	NSamp =0;
	NPack = 0;
	//variables to be set by po
	std::string args, file, type, ant, subdev, ref, wirefmt, mode;
	size_t total_num_samps, spb;
	double rate, freq, gain, bw, total_time, setup_time;

	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()("help", "help message")("args",
			po::value<std::string>(&args)->default_value(""),
			"multi uhd device address args")("file",
			po::value<std::string>(&file)->default_value(
					"usrp_samples_1995.dat"),
			"name of the file to write binary samples to")("mode",
			po::value<std::string>(&mode)->default_value("spec"),
			"Perform Spectrum of stream to file (spec/record/play)")("type",
			po::value<std::string>(&type)->default_value("short"),
			"sample type: double, float, or short")("nsamps",
			po::value<size_t>(&total_num_samps)->default_value(1000000),
			"total number of samples to receive")("duration",
			po::value<double>(&total_time)->default_value(0),
			"total number of seconds to receive")("spb",
			po::value<size_t>(&spb)->default_value(16384), "samples per buffer")(
			"rate", po::value<double>(&rate)->default_value(1e6),
			"rate of incoming samples")("freq",
			po::value<double>(&freq)->default_value(1500e6),
			"RF center frequency in Hz")("gain", po::value<double>(&gain),
			"gain for the RF chain")("ant", po::value<std::string>(&ant),
			"antenna selection")("subdev", po::value<std::string>(&subdev),
			"subdevice specification")("bw", po::value<double>(&bw),
			"analog frontend filter bandwidth in Hz")("ref",
			po::value<std::string>(&ref)->default_value("internal"),
			"reference source (internal, external, mimo)")("wirefmt",
			po::value<std::string>(&wirefmt)->default_value("sc16"),
			"wire format (sc8 or sc16)")("setup",
			po::value<double>(&setup_time)->default_value(1.0),
			"seconds of setup time")("loop", "Loop Playback")("progress",
			"periodically display short-term bandwidth")("stats",
			"show average bandwidth on exit")("sizemap",
			"track packet size and display breakdown on exit")("null",
			"run without writing to file")("continue",
			"don't abort on a bad packet")("skip-lo",
			"skip checking LO lock status")("int-n",
			"tune USRP with integer-N tuning");
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	//print the help message
	if (vm.count("help")) {
		std::cout << boost::format("UHD RX samples to file %s") % desc
				<< std::endl;
		std::cout << std::endl
				<< "This application streams data from a single channel of a USRP device to a file.\n"
				<< std::endl;
		return ~0;
	}
//
//    bool bw_summary = vm.count("progress") > 0;
//    bool stats = vm.count("stats") > 0;
//    bool null = vm.count("null") > 0;
//    bool enable_size_map = vm.count("sizemap") > 0;
//    bool continue_on_bad_packet = vm.count("continue") > 0;
//
//    if (enable_size_map)
//        std::cout << "Packet size tracking enabled - will only recv one packet at a time!" << std::endl;





    int policy, res;

    struct sched_param param;

    if ((policy = sched_getscheduler(getpid())) == -1)
    {
        perror("sched_getscheduler");
        exit(EXIT_FAILURE);
    }

    if ((res = sched_getparam(getpid(), &param)) == -1)
    {
        perror("sched_getparam");
        exit(EXIT_FAILURE);
    }

    std::cout << " ORIGINAL: ";
    std::cout << "policy=" << ((policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                               (policy == SCHED_RR)    ? "SCHED_RR" :
                               (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                                                          "???")
              << ", priority=" << param.sched_priority << std::endl;














	int OperationMode = -1;

	if (vm.count("mode")) {
		if (mode == "record") {
			OperationMode = 1;

		}
		if (mode == "play") {
			OperationMode = 2;
		}
		if (mode == "spec") {
			OperationMode = 0;
		}
	}

	if (OperationMode == -1)
		exit(-1);

	//create a usrp device
	std::cout << std::endl;
	std::cout << boost::format("Creating the usrp device with: %s...") % args
			<< std::endl;
	uhd::usrp::multi_usrp::sptr usrp = uhd::usrp::multi_usrp::make(args);

	//Lock mboard clocks
	usrp->set_clock_source(ref);
	bool PerformAGC = true;

	if (OperationMode == 2) { //Tx
		PerformAGC = false;
		//always select the subdevice first, the channel mapping affects the other settings
		if (vm.count("subdev"))
			usrp->set_tx_subdev_spec(subdev);

		std::cout << boost::format("Using Device: %s") % usrp->get_pp_string()
				<< std::endl;

		//set the sample rate
		if (not vm.count("rate")) {
			std::cerr << "Please specify the sample rate with --rate"
					<< std::endl;
			return ~0;
		}
		std::cout << boost::format("Setting TX Rate: %f Msps...") % (rate / 1e6)
				<< std::endl;
		usrp->set_tx_rate(rate);
		std::cout
				<< boost::format("Actual TX Rate: %f Msps...")
						% (usrp->get_tx_rate() / 1e6) << std::endl << std::endl;

		//set the center frequency
		if (not vm.count("freq")) {
			std::cerr << "Please specify the center frequency with --freq"
					<< std::endl;
			return ~0;
		}
		std::cout << boost::format("Setting TX Freq: %f MHz...") % (freq / 1e6)
				<< std::endl;
		uhd::tune_request_t tune_request;
		tune_request = uhd::tune_request_t(freq);
		tune_request.args = uhd::device_addr_t("mode_n=integer");
		usrp->set_tx_freq(tune_request);
		std::cout
				<< boost::format("Actual TX Freq: %f MHz...")
						% (usrp->get_tx_freq() / 1e6) << std::endl << std::endl;

		//set the rf gain
		if (vm.count("gain")) {
			std::cout << boost::format("Setting TX Gain: %f dB...") % gain
					<< std::endl;
			usrp->set_tx_gain(gain);
			std::cout
					<< boost::format("Actual TX Gain: %f dB...")
							% usrp->get_tx_gain() << std::endl << std::endl;
		}

		//set the analog frontend filter bandwidth
		if (vm.count("bw")) {
			std::cout << boost::format("Setting TX Bandwidth: %f MHz...") % bw
					<< std::endl;
			usrp->set_tx_bandwidth(bw);
			std::cout
					<< boost::format("Actual TX Bandwidth: %f MHz...")
							% usrp->get_tx_bandwidth() << std::endl
					<< std::endl;
		}

		boost::this_thread::sleep(boost::posix_time::seconds(1)); //allow for some setup time
		std::vector<std::string> sensor_names;
		sensor_names = usrp->get_tx_sensor_names(0);
		if (std::find(sensor_names.begin(), sensor_names.end(), "lo_locked")
				!= sensor_names.end()) {
			uhd::sensor_value_t lo_locked = usrp->get_tx_sensor("lo_locked", 0);
			std::cout
					<< boost::format("Checking TX: %s ...")
							% lo_locked.to_pp_string() << std::endl;
			UHD_ASSERT_THROW(lo_locked.to_bool());
		}

	} else {
		//always select the subdevice first, the channel mapping affects the other settings
		if (vm.count("subdev"))
			usrp->set_rx_subdev_spec(subdev);

		std::cout << boost::format("Using Device: %s") % usrp->get_pp_string()
				<< std::endl;

		//set the sample rate
		if (rate <= 0.0) {
			std::cerr << "Please specify a valid sample rate" << std::endl;
			return ~0;
		}
		std::cout << boost::format("Setting RX Rate: %f Msps...") % (rate / 1e6)
				<< std::endl;
		usrp->set_rx_rate(rate);
		std::cout
				<< boost::format("Actual RX Rate: %f Msps...")
						% (usrp->get_rx_rate() / 1e6) << std::endl << std::endl;

		// freq = 1600e6;
		//set the center frequency
		if (vm.count("freq")) { //with default of 0.0 this will always be true
			std::cout
					<< boost::format("Setting RX Freq: %f MHz...")
							% (freq / 1e6) << std::endl;
			uhd::tune_request_t tune_request(freq);
			if (vm.count("int-n"))
				tune_request.args = uhd::device_addr_t("mode_n=integer");
			usrp->set_rx_freq(tune_request);
			std::cout
					<< boost::format("Actual RX Freq: %f MHz...")
							% (usrp->get_rx_freq() / 1e6) << std::endl
					<< std::endl;
		}

		//set the rf gain

		//gain = 28.0;

		if (vm.count("gain")) {
			if (gain >= 0) {
				PerformAGC = false;
			} else {
				gain = 15;
			}
			std::cout << boost::format("Setting RX Gain: %f dB...") % gain
					<< std::endl;
			usrp->set_rx_gain(gain);
			std::cout
					<< boost::format("Actual RX Gain: %f dB...")
							% usrp->get_rx_gain() << std::endl << std::endl;

		}

		//set the IF filter bandwidth
		if (vm.count("bw")) {
			std::cout
					<< boost::format("Setting RX Bandwidth: %f MHz...")
							% (bw / 1e6) << std::endl;
			usrp->set_rx_bandwidth(bw);
			std::cout
					<< boost::format("Actual RX Bandwidth: %f MHz...")
							% (usrp->get_rx_bandwidth() / 1e6) << std::endl
					<< std::endl;
		}

		//set the antenna
		if (vm.count("ant"))
			usrp->set_rx_antenna(ant);

		boost::this_thread::sleep(boost::posix_time::seconds(setup_time)); //allow for some setup time

		//check Ref and LO Lock detect
		if (not vm.count("skip-lo")) {
			check_locked_sensor(usrp->get_rx_sensor_names(0), "lo_locked",
					boost::bind(&uhd::usrp::multi_usrp::get_rx_sensor, usrp, _1,
							0), setup_time);
			if (ref == "mimo")
				check_locked_sensor(usrp->get_mboard_sensor_names(0),
						"mimo_locked",
						boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor,
								usrp, _1, 0), setup_time);
			if (ref == "external")
				check_locked_sensor(usrp->get_mboard_sensor_names(0),
						"ref_locked",
						boost::bind(&uhd::usrp::multi_usrp::get_mboard_sensor,
								usrp, _1, 0), setup_time);
		}
	}
	Buffers objBuffers(SAMPS_PER_BUFF);
	StreamingRequests objReqQue;
	StreamerParams Params;

	Params.RequestsQue = (void *) &objReqQue;
	Params.usrp = usrp;
	Params.Buffer = (void *) &objBuffers;
	boost::thread workerThread;
	bool LoopMode = false;

	if (OperationMode == 2) {
		if (vm.count("loop")) {
			LoopMode = true;
		}
		cout << "Playing from file" << endl;
		unsigned long long CollectedSamples = 0;
		FILE *infile = fopen(file.c_str(), "rb");
		int BatchSize = SAMPS_PER_BUFF * 32;
		bool EndFile = false;
		while (objBuffers.NumBuffers > 0) {
			complex<short> *NewBuffer = objBuffers.GetMultipleWriteBuffer();
			int n = fread(NewBuffer, 4, BatchSize, infile);
			CollectedSamples += n;
			if (n < BatchSize) {
				EndFile = true;
				memset((void*) (NewBuffer + n), 0, (BatchSize - n) << 2);
				objBuffers.AdvanceWriteBuffer(32);

				if (LoopMode) {
					fseek(infile, 0, 3);
					EndFile = false;
				} else {
					break;
				}

			}

			objBuffers.AdvanceWriteBuffer(32);
		}

		//Start Streaming

		TxStreamer objStreamer(&Params);
		objStreamer.SetRate(rate);
		pobjSt = &objStreamer;
		objSpec.SetStreamer(&objBuffers, &objReqQue);
		boost::thread workerThread(RunStreamer, (void *) &objStreamer);

		  pthread_t threadID = (pthread_t) workerThread.native_handle();

		    struct sched_param param;
		    int retcode;
		    if ((retcode = pthread_getschedparam(threadID, &policy, &param)) != 0)
		    {
		        errno = retcode;
		        perror("pthread_getschedparam");
		        exit(EXIT_FAILURE);
		    }

		    std::cout << "INHERITED: ";
		    std::cout << "policy=" << ((policy == SCHED_FIFO)  ? "SCHED_FIFO" :
		                               (policy == SCHED_RR)    ? "SCHED_RR" :
		                               (policy == SCHED_OTHER) ? "SCHED_OTHER" :
		                                                         "???")
		              << ", priority=" << param.sched_priority << std::endl;

		    param.sched_priority = 55;

		        if ((retcode = pthread_setschedparam(threadID, policy, &param)) != 0)
		        {
		            errno = retcode;
		            perror("pthread_setschedparam");
		            exit(EXIT_FAILURE);
		        }



		cout<<"Read file again"<<endl;
		if (EndFile and not LoopMode) {
			fclose(infile);
		} else {
			if (EndFile and LoopMode) {
				fseek(infile, 0, 3);
			}

			while (not stop_signal_called) {
				while (1) {
					int NumBuffers = objBuffers.NumBuffers;

					if (NumBuffers > 32) {
						break;
					}

					boost::this_thread::sleep(boost::posix_time::milliseconds(1));

				}
				complex<short> *NewBuffer = objBuffers.GetMultipleWriteBuffer();
				int n = fread(NewBuffer, 4, BatchSize, infile);
				CollectedSamples += n;
				if((CollectedSamples &0xFFFFFF) == 0)
					cout<<"Read "<<CollectedSamples<<endl;
				if (n < BatchSize) {
					EndFile = true;
					memset((void*) (NewBuffer + n), 0, (BatchSize - n) << 2);
					objBuffers.AdvanceWriteBuffer(32);

					if (LoopMode) {
						fseek(infile, 0, 3);
					} else {
						break;
					}

				}

				objBuffers.AdvanceWriteBuffer(32);
			}

		}




		if(LoopMode)
		{
			fclose(infile);
		}

		while(not stop_signal_called)
			boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}

	else {

		Streamer objStreamer(&Params);
		objSpec.SetStreamer(&objBuffers, &objReqQue);






/*
	    policy = SCHED_RR;
	    param.sched_priority = 2;

	    if ((res = sched_setscheduler(getpid(), policy, &param)) == -1)
	    {
	        perror("sched_setscheduler");
	        exit(EXIT_FAILURE);
	    }

*/



		boost::thread workerThread(RunStreamer, (void *) &objStreamer);

		  pthread_t threadID = (pthread_t) workerThread.native_handle();

			    struct sched_param param;
			    int retcode;
			    if ((retcode = pthread_getschedparam(threadID, &policy, &param)) != 0)
			    {
			        errno = retcode;
			        perror("pthread_getschedparam");
			        exit(EXIT_FAILURE);
			    }

			    std::cout << "INHERITED: ";
			    std::cout << "policy=" << ((policy == SCHED_FIFO)  ? "SCHED_FIFO" :
			                               (policy == SCHED_RR)    ? "SCHED_RR" :
			                               (policy == SCHED_OTHER) ? "SCHED_OTHER" :
			                                                         "???")
			              << ", priority=" << param.sched_priority << std::endl;

			    policy = SCHED_FIFO;
			    param.sched_priority =  sched_get_priority_max(SCHED_FIFO);

			        if ((retcode = pthread_setschedparam(threadID, policy, &param)) != 0)
			        {
			            errno = retcode;
			            perror("pthread_setschedparam");
			            exit(EXIT_FAILURE);
			        }




//		displayAndChange(workerThread);


		if (PerformAGC) {
			//agc<std::complex<short> >(usrp, "sc16", wirefmt, file, spb, total_num_samps, total_time, bw_summary, stats, null, enable_size_map, continue_on_bad_packet,gain);
			AGC2(usrp, gain, &objBuffers, &objReqQue);
		}

		//  recv_to_file<std::complex<short> >(usrp, "sc16", wirefmt, file, spb, total_num_samps, total_time, bw_summary, stats, null, enable_size_map, continue_on_bad_packet);

		if (OperationMode == 0) {
			cout << "Creating Spectrum" << endl;
			objSpec.CreateOutput(file.c_str(), float(rate), float(freq));
		} else if (OperationMode == 1) {
			cout << "Saving to file" << endl;
			objSpec.Record2File(file.c_str(), total_num_samps);
			cout << "Finished File" << endl;
		}
		stop_signal_called = true;
	}

	workerThread.join();
	cout << "Finished" << endl;

//float *y = aligned_alloc(32, 1024*sizeof(float));

	return 0;
}
