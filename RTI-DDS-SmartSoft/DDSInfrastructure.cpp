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

#include "RtiDdsSmartSoft/DDSInfrastructure.h"

namespace SmartDDS {

DDSInfrastructure::DDSInfrastructure(const int domain_id)
:	domain_participant(domain_id)
,	correlationid_filter(new CorrelationIdFilter())
{
	domain_participant->register_contentfilter(correlationid_filter, CorrelationIdFilter::DEFAULT_FILTER_NAME);
}

DDSInfrastructure::~DDSInfrastructure()
{
	domain_participant->unregister_contentfilter(CorrelationIdFilter::DEFAULT_FILTER_NAME);
	domain_participant.close();
	dds::domain::DomainParticipant::finalize_participant_factory();
}

const dds::domain::DomainParticipant& DDSInfrastructure::getDomainParticipant() const
{
	return domain_participant;
}

DynamicDataTopic DDSInfrastructure::findOrCreateTopic(
			const std::string &topicName,
			const DynamicStructType &dynamicType)
{
	std::unique_lock<std::mutex> scoped_lock(infrastructure_mutex);
	auto dds_topic = dds::topic::find<DynamicDataTopic>(domain_participant, topicName);
	if(dds_topic.is_nil()) {
		dds_topic = DynamicDataTopic(domain_participant, topicName, dynamicType);
	}
	return dds_topic;
}

DynamicDataFilteredTopic DDSInfrastructure::findOrCreateClientFilteredTopic(
			const DynamicDataTopic &parent_topic,
			const ConnectionId &id, const std::string &filter_parameter)
{
	std::unique_lock<std::mutex> scoped_lock(infrastructure_mutex);
	std::string cft_name = parent_topic.name() + "::Filtered_"+id.toString();
	auto dds_topic = dds::topic::find<DynamicDataFilteredTopic>(parent_topic.participant(), cft_name);
	if(dds_topic.is_nil()) {
		auto filter = CorrelationIdFilter::createClientFilter(id);
		if(filter_parameter != "") {
			filter.add_parameter(filter_parameter);
		}
		dds_topic = DynamicDataFilteredTopic(parent_topic, cft_name, filter);
	}
	return dds_topic;
}

void DDSInfrastructure::resetTopic(DynamicDataTopic &topic)
{
	std::unique_lock<std::mutex> scoped_lock(infrastructure_mutex);
	// reset the topic reference
	topic = nullptr;
}

void DDSInfrastructure::resetFilteredTopic(DynamicDataFilteredTopic &filtered_topic)
{
	std::unique_lock<std::mutex> scoped_lock(infrastructure_mutex);
	// reset the topic reference
	filtered_topic = nullptr;
}

} /* namespace SmartDDS */
