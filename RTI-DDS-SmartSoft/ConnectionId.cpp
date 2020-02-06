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

#include <sstream>
#include "RtiDdsSmartSoft/ConnectionId.h"

namespace SmartDDS {

ConnectionId::ConnectionId()
:	connection_id(rti::core::Guid::unknown())
{  }

ConnectionId::ConnectionId(const rti::core::Guid &guid)
:	connection_id(guid)
{  }

ConnectionId::ConnectionId(const std::string &string_id)
:	connection_id()
{
	for(size_t i=0; i<rti::core::Guid::LENGTH; ++i) {
		// get a pair of digits representing one hex number
		std::string hex_str = string_id.substr(i*2, 2);
		// convert the current two digits to a hex integer
		connection_id[i] = std::stoi(hex_str, nullptr, 16);
	}
}
ConnectionId::ConnectionId(const std::vector<uint8_t> &vector_id)
:	connection_id()
{
	if(vector_id.size() == rti::core::Guid::LENGTH) {
		for(size_t i=0; i<rti::core::Guid::LENGTH; ++i) {
			connection_id[i] = vector_id[i];
		}
	}
}

ConnectionId::ConnectionId(const DynamicDataReader &related_reader)
{
	connection_id = related_reader.qos().policy<rti::core::policy::DataReaderProtocol>().virtual_guid();
}
ConnectionId::ConnectionId(const DynamicDataWriter &related_writer)
{
	connection_id = related_writer.qos().policy<rti::core::policy::DataWriterProtocol>().virtual_guid();
}

ConnectionId::operator const rti::core::Guid&() const
{
	return connection_id;
}

bool ConnectionId::isUnknown() const
{
	return connection_id == rti::core::Guid::unknown();
}

bool ConnectionId::operator<(const ConnectionId &other) const
{
	return connection_id < other.connection_id;
}
bool ConnectionId::operator==(const ConnectionId &other) const
{
	return connection_id == other.connection_id;
}
bool ConnectionId::operator!=(const ConnectionId &other) const
{
	return connection_id != other.connection_id;
}

std::string ConnectionId::toString() const
{
	std::stringstream connection_id_ss;
	connection_id_ss << std::hex;
	for(size_t i=0; i<rti::core::Guid::LENGTH; ++i) {
		connection_id_ss << std::setw(2) << std::setfill('0') << (int)connection_id[i];
	}
	return connection_id_ss.str();
}

std::vector<uint8_t> ConnectionId::toVector() const
{
	std::vector<uint8_t> connection_id_vector(rti::core::Guid::LENGTH);
	for(size_t i=0; i<rti::core::Guid::LENGTH; ++i) {
		connection_id_vector[i] = connection_id[i];
	}
	return connection_id_vector;
}

} /* namespace SmartDDS */
