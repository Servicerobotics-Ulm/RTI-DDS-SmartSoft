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

#ifndef RTIDDSSMARTSOFT_DDSREADERCONNECTOR_H_
#define RTIDDSSMARTSOFT_DDSREADERCONNECTOR_H_

#include <smartStatusCode.h>
#include <smartChronoAliases.h>

#include "RTI-DDS-SmartSoft/Component.h"
#include "RTI-DDS-SmartSoft/DDSAliases.h"

namespace SmartDDS {

class DDSReaderConnector
:	public DynamicDataReaderListener
{
private:
	Component* component;
	dds::sub::qos::DataReaderQos reader_qos;
	dds::core::cond::GuardCondition connection_guard;
	dds::core::cond::GuardCondition incompatible_qos_guard;

	dds::sub::Subscriber create_subscriber() const;

	Smart::StatusCode wait_for_connection(
			DynamicDataReader & dds_reader,
			const Smart::Duration & timeout,
			DynamicDataReaderListener* the_listener,
			const dds::core::status::StatusMask& mask
			);

	/**
	 * @brief Handles the dds::core::status::RequestedIncompatibleQosStatus status
	 *
	 * @details \dref_details_DataReaderListener_on_requested_incompatible_qos
	 */
	virtual void on_requested_incompatible_qos(
			DynamicDataReader& reader,
			const dds::core::status::RequestedIncompatibleQosStatus& status) override;
	/**
	 * @brief Handles the dds::core::status::SubscriptionMatchedStatus status
	 *
	 * @details \dref_details_DataReaderListener_on_subscription_matched
	 */
	virtual void on_subscription_matched(
			DynamicDataReader& reader,
			const dds::core::status::SubscriptionMatchedStatus& status) override;

public:
	DDSReaderConnector(Component* component, const dds::topic::qos::TopicQos &topic_qos);
	virtual ~DDSReaderConnector() = default;

	void reset(DynamicDataReader &dds_reader);

	template <typename TopicType>
	DynamicDataReader create_new_reader(
			const TopicType &dds_topic,
			DynamicDataReaderListener* the_listener = NULL,
			const dds::core::status::StatusMask& mask = dds::core::status::StatusMask::all())
	{
		try {
			return DynamicDataReader(create_subscriber(), dds_topic, reader_qos, the_listener, mask);
		} catch(dds::core::Error &error) {
			std::cerr << error.what() << std::endl;
		}
		return DynamicDataReader(nullptr);
	}

	template <typename TopicType>
	Smart::StatusCode reconnect(
			DynamicDataReader & dds_reader,
			const TopicType & dds_topic,
			const Smart::Duration & timeout = Smart::Duration::zero(),
			DynamicDataReaderListener* the_listener = NULL,
			const dds::core::status::StatusMask& mask = dds::core::status::StatusMask::all())
	{
		//reset both guards
		connection_guard.trigger_value(false);
		incompatible_qos_guard.trigger_value(false);
		// create a new reader whose connection status is not yet acknowledged
		dds_reader = create_new_reader(dds_topic, this);
		if(dds_reader.is_nil()) {
			return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
		} else {
			// wait till reader acknowledges that there is a remote endpoint or otherwise a related status code is returned
			return wait_for_connection(dds_reader, timeout, the_listener, mask);
		}
	}
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_DDSREADERCONNECTOR_H_ */
