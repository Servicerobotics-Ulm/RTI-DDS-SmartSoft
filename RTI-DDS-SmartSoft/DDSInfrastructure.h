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

#ifndef RTIDDSSMARTSOFT_DDSINFRASTRUCTURE_H_
#define RTIDDSSMARTSOFT_DDSINFRASTRUCTURE_H_

#include <map>

#include <dds/dds.hpp>

#include "RTI-DDS-SmartSoft/ConnectionId.h"
#include "RTI-DDS-SmartSoft/CorrelationIdFilter.h"

namespace SmartDDS {

class DDSInfrastructure {
private:
	std::mutex infrastructure_mutex;
	dds::domain::DomainParticipant domain_participant;
	rti::topic::CustomFilter<CorrelationIdFilter> correlationid_filter;

public:
	DDSInfrastructure(const int domain_id = 0);
	virtual ~DDSInfrastructure();

	/** get shared domain participant reference
	 * @return domain participant const reference
	 */
	const dds::domain::DomainParticipant& getDomainParticipant() const;

	DynamicDataTopic findOrCreateTopic(
				const std::string &topicName,
				const DynamicStructType &dynamicType);

	DynamicDataFilteredTopic findOrCreateClientFilteredTopic(
				const DynamicDataTopic &parent_topic,
				const ConnectionId &id, const std::string &filter_parameter = "");

	void resetTopic(DynamicDataTopic &topic);
	void resetFilteredTopic(DynamicDataFilteredTopic &topic);
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_DDSINFRASTRUCTURE_H_ */
