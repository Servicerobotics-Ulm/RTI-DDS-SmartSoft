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

#ifndef CORRELATIONIDFILTER_H_
#define CORRELATIONIDFILTER_H_

#include <list>
#include <vector>
#include <shared_mutex>

#include "RTI-DDS-SmartSoft/DDSAliases.h"
#include "RTI-DDS-SmartSoft/CorrelationId.h"

namespace SmartDDS {

struct CompiledReaderData {
	std::list<CompiledReaderData>::iterator self_index;
	CorrelationId correlation_id;
	rti::core::SequenceNumber prescale_factor = rti::core::SequenceNumber::unknown();
};

class CorrelationIdFilter
:	public rti::topic::ContentFilter<DynamicDataSample, CompiledReaderData>
{
public:
	virtual ~CorrelationIdFilter() = default;
	static std::string DEFAULT_FILTER_NAME;
	static dds::topic::Filter createClientFilter(const ConnectionId &connection_id);
private:
	mutable std::shared_timed_mutex writer_mutex;
	std::list<CompiledReaderData> compiled_readers;

    virtual CompiledReaderData& compile(
        const std::string& expression,
        const dds::core::StringSeq& parameters,
        const dds::core::optional<dds::core::xtypes::DynamicType>& type_code,
        const std::string& type_class_name,
		CompiledReaderData *old_compile_data) override;

    virtual bool evaluate(
    	CompiledReaderData& compile_data,
        const DynamicDataSample& sample,
        const rti::topic::FilterSampleInfo& meta_data) override;

    virtual void finalize(CompiledReaderData& compile_data) override;
};

} /* namespace SmartDDS */

#endif /* CORRELATIONIDFILTER_H_ */
