/*
 * defs.h
 *
 *  Created on: Oct 26, 2017
 *      Author: x300
 */

#ifndef DEFS_H_
#define DEFS_H_

#define VERSION 1.0
#define HEADER_LENGTH 8*5
//#define SERVER_VERSION
//define ETHERNET_10G
#include <uhd/types/tune_request.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/exception.hpp>
#include <iostream>


#include <fstream>
#include <csignal>
#include <complex>


#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>


#define MFFT 200
#define NFFT 32768
#ifdef ETHERNET_10G
	#define SAMPS_PER_BUFF 7984
	#define ONE_BATCH 1996
	#define NUM_BATCHES 4

#else
	#define SAMPS_PER_BUFF 7986

	#define NUM_BATCHES 22
	#define ONE_BATCH 363
#endif

#define TIME2PRINT 2

struct StreamerParams
{
	   uhd::usrp::multi_usrp::sptr usrp;
	   void * Buffer;
	   void * RequestsQue;
};


#define MIN_TX_RATE 1e6
#define TX_K_FC_MAX 1.5
#define TX_K_FC_MIN 1.25

#endif /* DEFS_H_ */
