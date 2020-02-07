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

#include <ostream>
#include "RTI-DDS-SmartSoft/CorrelationId.h"

namespace SmartDDS {

CorrelationId::CorrelationId()
:	writer_handle(nullptr)
,	sample_id(rti::core::SampleIdentity::automatic())
{  }
CorrelationId::CorrelationId(const rti::core::SampleIdentity &sid)
:	writer_handle(nullptr)
,	sample_id(sid)
{  }
CorrelationId::CorrelationId(const dds::sub::SampleInfo &info)
:	writer_handle(info.publication_handle())
,	sample_id(info->original_publication_virtual_sample_identity())
{  }

CorrelationId::CorrelationId(const ConnectionId &connection_id, const rti::core::SequenceNumber &sequence_number)
:	writer_handle(nullptr)
,	sample_id(connection_id, sequence_number)
{  }

CorrelationId CorrelationId::createRelatedId(const dds::sub::SampleInfo &info) {
	CorrelationId relatedId;
	relatedId.writer_handle = info.publication_handle();
	relatedId.sample_id = info->related_original_publication_virtual_sample_identity();
	return relatedId;
}

CorrelationId::operator const rti::core::SampleIdentity&() const {
	return sample_id;
}

ConnectionId CorrelationId::getConnectionId() const {
	return ConnectionId(sample_id.writer_guid());
}

const dds::core::InstanceHandle& CorrelationId::getWriterHandle() const {
	return writer_handle;
}

const rti::core::SequenceNumber& CorrelationId::getSequenceNumber() const {
	return sample_id.sequence_number();
}

CorrelationId
CorrelationId::operator++(int)
{
	if(sample_id.sequence_number() >= rti::core::SequenceNumber::zero()) {
		sample_id.sequence_number()++;
	}
	return *this;
}

bool CorrelationId::less_than(const Smart::ICorrelationId *other) const
{
	auto cid_other = dynamic_cast<const CorrelationId*>(other);
	if(cid_other) {
		if(sample_id.writer_guid() == cid_other->sample_id.writer_guid()) {
			return sample_id.sequence_number() < cid_other->sample_id.sequence_number();
		} else {
			return sample_id.writer_guid() < cid_other->sample_id.writer_guid();
		}
	}
	return false;
}

bool CorrelationId::equals_to(const Smart::ICorrelationId *other) const
{
	auto cid_other = dynamic_cast<const CorrelationId*>(other);
	if(cid_other) {
		if(sample_id.writer_guid() == cid_other->sample_id.writer_guid()
				&& sample_id.sequence_number() == cid_other->sample_id.sequence_number())
		{
			return true;
		}
	}
	return false;
}

std::string CorrelationId::to_string() const
{
	std::stringstream sstream;
	sstream << sample_id;
	return sstream.str();
}

} /* namespace SmartDDS */
