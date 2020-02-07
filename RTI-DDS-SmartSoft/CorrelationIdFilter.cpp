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

#include "RTI-DDS-SmartSoft/CorrelationIdFilter.h"

namespace SmartDDS {

std::string CorrelationIdFilter::DEFAULT_FILTER_NAME = "SmartDDS::Filter::CorrelationId";

dds::topic::Filter CorrelationIdFilter::createClientFilter(const ConnectionId &connection_id)
{
	dds::topic::Filter filter(connection_id.toString());
	filter->name(DEFAULT_FILTER_NAME);
	return filter;
}

CompiledReaderData& CorrelationIdFilter::compile(
    const std::string& expression,
    const dds::core::StringSeq& parameters,
    const dds::core::optional<dds::core::xtypes::DynamicType>& type_code,
    const std::string& type_class_name,
	CompiledReaderData *old_compile_data)
{
	std::unique_lock<std::shared_timed_mutex> writer_lock(writer_mutex);

	CompiledReaderData reader_data;

	// we extract the connection ID from the expression and initialize the sequence-counter to zero
	reader_data.correlation_id = CorrelationId(ConnectionId(expression), rti::core::SequenceNumber::zero());

	// extract the optional prescale factor from the parameters list
	if(parameters.size() > 0) {
		std::stringstream ss_id(parameters[0]);
		int prescale_factor = 0;
		ss_id >> prescale_factor;
		if(prescale_factor > 0) {
			reader_data.prescale_factor = rti::core::SequenceNumber(prescale_factor);
		}
	}

	compiled_readers.push_front(reader_data);
	compiled_readers.front().self_index = compiled_readers.begin();
	return compiled_readers.front();
}

bool CorrelationIdFilter::evaluate(
	CompiledReaderData& compile_data,
    const DynamicDataSample& sample,
    const rti::topic::FilterSampleInfo& meta_data)
{
	std::unique_lock<std::shared_timed_mutex> writer_lock(writer_mutex);
	if(compile_data.prescale_factor != rti::core::SequenceNumber::unknown()) {
		auto current_update_value = compile_data.correlation_id.getSequenceNumber().value();
		// increment the reader's update counter
		compile_data.correlation_id++;
		if(current_update_value % compile_data.prescale_factor.value() == 0) {
			return true;
		}
	} else {
		// no prescale factor has been defined so we use the related sample ID to pass this sample
		// only to the related client connection
		if(compile_data.correlation_id.getConnectionId() == meta_data.related_sample_identity().writer_guid()) {
			return true;
		}
	}
	return false;
}

void CorrelationIdFilter::finalize(CompiledReaderData& compile_data)
{
	std::unique_lock<std::shared_timed_mutex> writer_lock(writer_mutex);
	compiled_readers.erase(compile_data.self_index);
}

} /* namespace SmartDDS */
