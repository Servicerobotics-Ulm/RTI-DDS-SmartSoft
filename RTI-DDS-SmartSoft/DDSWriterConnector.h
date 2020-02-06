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

#ifndef RTIDDSSMARTSOFT_DDSWRITERCONNECTOR_H_
#define RTIDDSSMARTSOFT_DDSWRITERCONNECTOR_H_

#include <smartStatusCode.h>
#include <smartChronoAliases.h>

#include "RtiDdsSmartSoft/Component.h"
#include "RtiDdsSmartSoft/DDSAliases.h"

namespace SmartDDS {

class DDSWriterConnector
:	public DynamicDataWriterListener
{
private:
	Component* component;
	dds::topic::qos::TopicQos topic_qos;
	dds::core::cond::GuardCondition connection_guard;

    virtual void on_publication_matched(DynamicDataWriter &writer, const PublicationMatchedStatus &status) override;

public:
    DDSWriterConnector(Component* component, const dds::topic::qos::TopicQos &topic_qos);
	virtual ~DDSWriterConnector() = default;

	void reset(DynamicDataWriter &dds_writer) const;

	DynamicDataWriter create_new_writer(
			const DynamicDataTopic &dds_topic,
			DynamicDataWriterListener* the_listener = NULL,
			const dds::core::status::StatusMask& mask = dds::core::status::StatusMask::all()) const;

	Smart::StatusCode reconnect(
			DynamicDataWriter & dds_writer,
			const DynamicDataTopic & dds_topic,
			const Smart::Duration & timeout = Smart::Duration::max(),
			DynamicDataWriterListener* the_listener = NULL,
			const dds::core::status::StatusMask& mask = dds::core::status::StatusMask::all());
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_DDSWRITERCONNECTOR_H_ */
