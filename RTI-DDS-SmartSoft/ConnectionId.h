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

#ifndef RTIDDSSMARTSOFT_CONNECTIONID_H_
#define RTIDDSSMARTSOFT_CONNECTIONID_H_

#include <string>
#include <vector>
#include <iostream>

#include "RtiDdsSmartSoft/DDSAliases.h"

namespace SmartDDS {

class ConnectionId {
private:
	rti::core::Guid connection_id;
public:
	ConnectionId();
	ConnectionId(const rti::core::Guid &guid);
	ConnectionId(const std::string &string_id);
	ConnectionId(const std::vector<uint8_t> &vector_id);

	// initiate connection from the related reader/writer
	ConnectionId(const DynamicDataReader &related_reader);
	ConnectionId(const DynamicDataWriter &related_writer);

	// default copy/move constructors and assignment operators
	ConnectionId(const ConnectionId&) = default;
	ConnectionId(ConnectionId&&) = default;
	ConnectionId& operator=(const ConnectionId&) = default;
	ConnectionId& operator=(ConnectionId&&) = default;

	operator const rti::core::Guid&() const;

	bool isUnknown() const;

	bool operator<(const ConnectionId &other) const;
	bool operator==(const ConnectionId &other) const;
	bool operator!=(const ConnectionId &other) const;

	std::string toString() const;
	std::vector<uint8_t> toVector() const;
};

inline std::ostream& operator<<(std::ostream& out, const ConnectionId& cid)
{
    out << static_cast<const rti::core::Guid&>(cid);
    return out;
}

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_CONNECTIONID_H_ */
