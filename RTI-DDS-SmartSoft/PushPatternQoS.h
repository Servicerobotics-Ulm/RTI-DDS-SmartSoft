//===================================================================================
//
//  Copyright (C) 2019 Alex Lotz
//
//        lotz@hs-ulm.de
//
//        Servicerobotik Ulm
//        Christian Schlegel
//        Ulm University of Applied Sciences
//        Prittwitzstr. 10
//        89075 Ulm
//        Germany
//
//  This file is part of the SmartSoft Component-Developer C++ API.
//
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
//  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGE.
//
//===================================================================================

#ifndef RTIDDSSMARTSOFT_PUSHPATTERNQOS_H_
#define RTIDDSSMARTSOFT_PUSHPATTERNQOS_H_

#include <dds/dds.hpp>

#include <smartChronoAliases.h>

namespace SmartDDS {

class PushPatternQoS {
public:
	static dds::topic::qos::TopicQos getTopicQoS() {
		dds::topic::qos::TopicQos topic_qos;
		// Reliable QoS means that no packets can be lost, even when using unreliable transport mechanisms
		topic_qos << dds::core::policy::Reliability::Reliable();
		// History=KeepLast(1) means the reader has an internal buffer of one element
		// this buffered value can be read between updates
		topic_qos << dds::core::policy::History::KeepLast(1);
		// Exclusive Ownership means there can only be one DataWriter (for a certain Topic) at a time
		// the typical behavior is that the latest started writer becomes the dominant one
		// and all the previously started writers become inactive
		topic_qos << dds::core::policy::Ownership::Exclusive();
		// Durability=Volatile means that only when a reader is connected to an active writer, and
		// the writer calls its write method, only then data is actually transmitted.
		// As a consequence, when a new reader connects to a writer, it needs to wait until
		// the writer calls its write method next time, and only then the reader will get its first value
		topic_qos << dds::core::policy::Durability::Volatile();
		// an alternative durability QoS might be "TransientLocal", which would mean that
		// a writer itself keeps a local copy of the latest object and whenever a new
		// reader connects, the writer would automatically and instantly re-send the last data value
		// even before the next time the write method is called.
		// The drawback of this option would be that after a reader connects, the first two
		// data updates might come very fast after each other and by that violate the maximal "periodic"
		// update frequency of the reader (if the reader has configured one)
		// therefore we use the volatile durability by default as it is more predictive
//		topic_qos << dds::core::policy::Durability::TransientLocal();
		// Automatic Lifeliness means that the middleware by itself determines when and how often
		// to check for alive end-points (no manual user interaction required)
		topic_qos << dds::core::policy::Liveliness::Automatic();
		return topic_qos;
	}

	static dds::pub::qos::DataWriterQos getWriterQoS(const Smart::Duration &cycleTime = Smart::Duration::zero()) {
		auto writer_qos = dds::core::QosProvider::Default().datawriter_qos();
		// we use the TopicQoS as a reference for the majority of writer's QoS policies
		writer_qos = getTopicQoS();
		// the only writer-specific policy is the Deadline QoS
		if (cycleTime != Smart::Duration::zero()) {
			// cycleTime != zero means we have a periodic writer with a given update period (aka cycle-time)
			// now we use the Deadline policy to tell DDS the commitment to publish with this period
			// however, as we are using best effort systems in most cases, the jitter of scheduling effects
			// would likely violate this deadline if we would set the deadline exactly to the cycle time.
			// If we however consider the kind of worst-case scenario then a reasonable value for the duration
			// seems to be the double cycle-time (i.e. 2 x cycle-time), and that is what we set here.
			dds::core::Duration deadlinePeriod = 2 * cycleTime;
			writer_qos << dds::core::policy::Deadline(deadlinePeriod);
		}
		return writer_qos;
	}
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_PUSHPATTERNQOS_H_ */
