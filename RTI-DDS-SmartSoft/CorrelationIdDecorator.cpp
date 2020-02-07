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

#include <vector>

#include "RTI-DDS-SmartSoft/CorrelationIdDecorator.h"

namespace SmartDDS {

std::string CorrelationIdDecorator::CONNECTION_ID_NAME = "connection_id";
std::string CorrelationIdDecorator::SEQUENCE_NUMBER_NAME = "sequence_number";

CorrelationIdDecorator::CorrelationIdDecorator(const DynamicStructType &original_dds_type)
:	decorated_dds_type(original_dds_type.name()+"::DecoratedObject")
{
	using namespace dds::core::xtypes;
	decorated_dds_type.add_member(Member(CONNECTION_ID_NAME, SequenceType(primitive_type<uint8_t>(), rti::core::Guid::LENGTH)).key(true));
	decorated_dds_type.add_member(Member(SEQUENCE_NUMBER_NAME, primitive_type<long long>()));
	decorated_dds_type.add_member(Member("original_data", original_dds_type));
}

DynamicStructType CorrelationIdDecorator::getDecoratedDDSType() const
{
	return decorated_dds_type;
}

DynamicDataSample CorrelationIdDecorator::createDecoratedObject(
		const CorrelationId &correlattion_id,
		const DynamicDataSample &original_object) const
{
	DynamicDataSample decorated_object(decorated_dds_type);

	decorated_object.set_values(CONNECTION_ID_NAME, correlattion_id.getConnectionId().toVector());
	decorated_object.value(SEQUENCE_NUMBER_NAME, correlattion_id.getSequenceNumber().value());
	decorated_object.value("original_data", original_object);

	return decorated_object;
}

CorrelationId CorrelationIdDecorator::extractCorrelationId(const DynamicDataSample &decorated_object)
{
	ConnectionId connection_id ( decorated_object.get_values<uint8_t>(CONNECTION_ID_NAME) );
	auto sequence_number = decorated_object.value<long long>(SEQUENCE_NUMBER_NAME);
	return rti::core::SampleIdentity(connection_id, sequence_number);
}

DynamicDataSample CorrelationIdDecorator::extractOriginalObject(const DynamicDataSample &decorated_object)
{
	return decorated_object.value<DynamicDataSample>("original_data");
}

} /* namespace SmartDDS */
