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

#ifndef RTIDDSSMARTSOFT_CORRELATIONID_H_
#define RTIDDSSMARTSOFT_CORRELATIONID_H_

#include <dds/dds.hpp>

#include <smartICorrelationId.h>

#include "RtiDdsSmartSoft/ConnectionId.h"

namespace SmartDDS {

class CorrelationId : public Smart::ICorrelationId {
private:
	rti::core::SampleIdentity sample_id;
	dds::core::InstanceHandle writer_handle;

protected:
	virtual bool less_than(const ICorrelationId *other) const override final;
	virtual bool equals_to(const ICorrelationId *other) const override final;

public:
	CorrelationId();
	CorrelationId(const dds::sub::SampleInfo &info);
	CorrelationId(const rti::core::SampleIdentity &sid);
	CorrelationId(const ConnectionId &connection_id, const rti::core::SequenceNumber &sequence_number);

	static CorrelationId createRelatedId(const dds::sub::SampleInfo &info);

	// default copy/move constructors and assignment operators
	CorrelationId(const CorrelationId&) = default;
	CorrelationId(CorrelationId&&) = default;
	CorrelationId& operator=(const CorrelationId&) = default;
	CorrelationId& operator=(CorrelationId&&) = default;

	operator const rti::core::SampleIdentity&() const;

	ConnectionId getConnectionId() const;

	const dds::core::InstanceHandle& getWriterHandle() const;
	const rti::core::SequenceNumber& getSequenceNumber() const;

	// post increment operator
	CorrelationId operator++(int);

	virtual std::string to_string() const override final;
};

inline std::ostream& operator<<(std::ostream& out, const CorrelationId& cid)
{
    out << static_cast<const rti::core::SampleIdentity&>(cid);
    return out;
}

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_CORRELATIONID_H_ */
