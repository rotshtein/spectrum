/*
 * TxStreamer.h
 *
 *  Created on: Nov 6, 2017
 *      Author: x300
 */

#ifndef TXSTREAMER_H_
#define TXSTREAMER_H_

#include "Streamer.h"
#include <uhd/convert.hpp>

class TxStreamer: public Streamer {
short *ZeroBuffer;
public:
	TxStreamer(struct StreamerParams *Params);
	virtual ~TxStreamer();
	void Run(void);
};

#endif /* TXSTREAMER_H_ */
