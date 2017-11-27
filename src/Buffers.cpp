/*
 * Buffers.cpp
 *
 *  Created on: Oct 31, 2017
 *      Author: x300
 */

#include "Buffers.h"

Buffers::Buffers(int samps_per_buff) {
	// TODO Auto-generated constructor stub

	BigBuffer = (complex<short>*) aligned_alloc(32, NUM_BUFFERS*samps_per_buff*sizeof(complex<short>));

	for (int ii = 0; ii < NUM_BUFFERS; ii++) {
		Buffers1[ii] =  & (BigBuffer[ii*samps_per_buff]);
	}

}

Buffers::~Buffers() {
	// TODO Auto-generated destructor stub

	free(BigBuffer);

}

