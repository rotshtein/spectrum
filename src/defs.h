/*
 * defs.h
 *
 *  Created on: Oct 26, 2017
 *      Author: x300
 */

#ifndef DEFS_H_
#define DEFS_H_


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
#define SAMPS_PER_BUFF 8192


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
