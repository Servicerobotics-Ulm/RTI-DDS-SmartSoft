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

#ifndef RTIDDSSMARTSOFT_QUERYPATTERNQOS_H_
#define RTIDDSSMARTSOFT_QUERYPATTERNQOS_H_

#include <dds/dds.hpp>

namespace SmartDDS {

class QueryPatternQoS {
public:
	static dds::topic::qos::TopicQos getRequestTopicQoS()
	{
		dds::topic::qos::TopicQos topic_qos;
		// Reliable QoS means that no packets can be lost, even when using unreliable transport mechanisms
		topic_qos << dds::core::policy::Reliability::Reliable();
		// Shared Ownership means multiple writers (e.g. multiple QueryClients) can write data
		// to the same reader (e.g. QueryServer)
		topic_qos << dds::core::policy::Ownership::Shared();
		// we will not drop or override any (not yet handled) values, but the reader will consume the values
		// using reader.take_requests method; this implicates that if the reader cannot drain its queue
		// fast enough, the internal reader queue will build up over time
		topic_qos << dds::core::policy::History::KeepAll();
		// the Volatile Durability strategy means that the writer does not buffer its previous values
		// so if a QueryClient is not connected to any QueryServer, it does not make sense to keep
		// any pending requests as they are invalid anyways
		topic_qos << dds::core::policy::Durability::Volatile();
		// Automatic Lifeliness means that the middleware by itself determines when and how often
		// to check for alive end-points (no manual user interaction required)
		topic_qos << dds::core::policy::Liveliness::Automatic();
		return topic_qos;
	}

	static dds::topic::qos::TopicQos getReplyTopicQoS()
	{
		dds::topic::qos::TopicQos topic_qos;
		// Reliable QoS means that no packets can be lost, even when using unreliable transport mechanisms
		topic_qos << dds::core::policy::Reliability::Reliable();
		// Exclusive Ownership means there can only be one DataWriter (for a certain Topic) at a time
		// the typical behavior is that the latest started writer becomes the dominant one
		// and all the previously started writers become inactive
		// In other words, we can only have one QueryServer as a replier
		topic_qos << dds::core::policy::Ownership::Exclusive();
		// History=KeepAll means that a QueryClient will buffer all incoming query answers until they are consumed
		// one after the other by the QueryClient (or when the QueryClient discards individual pending queries)
		topic_qos << dds::core::policy::History::KeepAll();
		// the Volatile Durability means that a QueryServer does not buffer query answers for already disconnected QueryClients
		// answers are considered invalid if the requesting client disappears
		topic_qos << dds::core::policy::Durability::Volatile();
		// Automatic Lifeliness means that the middleware by itself determines when and how often
		// to check for alive end-points (no manual user interaction required)
		topic_qos << dds::core::policy::Liveliness::Automatic();
		return topic_qos;
	}
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_QUERYPATTERNQOS_H_ */
