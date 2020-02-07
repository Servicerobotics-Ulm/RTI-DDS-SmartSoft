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


#include "RTI-DDS-SmartSoft/DDSReaderConnector.h"

namespace SmartDDS {
	/**
	 * @brief Handles the dds::core::status::RequestedIncompatibleQosStatus status
	 *
	 * @details \dref_details_DataReaderListener_on_requested_incompatible_qos
	 */
	void DDSReaderConnector::on_requested_incompatible_qos(
			DynamicDataReader& reader,
			const dds::core::status::RequestedIncompatibleQosStatus& status)
	{
		incompatible_qos_guard.trigger_value(true);
	}

	/**
	 * @brief Handles the dds::core::status::SubscriptionMatchedStatus status
	 *
	 * @details \dref_details_DataReaderListener_on_subscription_matched
	 */
	void DDSReaderConnector::on_subscription_matched(
			DynamicDataReader& reader,
			const dds::core::status::SubscriptionMatchedStatus& status)
	{
		if(status.total_count() > 0) {
			connection_guard.trigger_value(true);
		}
	}

	dds::sub::Subscriber DDSReaderConnector::create_subscriber() const
	{
		return dds::sub::Subscriber(component->DDS().getDomainParticipant());
	}

	DDSReaderConnector::DDSReaderConnector(Component* component, const dds::topic::qos::TopicQos &topic_qos)
	:	component(component)
	{
		reader_qos = dds::core::QosProvider::Default().datareader_qos();
		// we use the topic QoS to specify the reader QoS
		reader_qos = topic_qos;
	}

	Smart::StatusCode DDSReaderConnector::wait_for_connection(
			DynamicDataReader & dds_reader,
			const Smart::Duration & timeout,
			DynamicDataReaderListener* the_listener,
			const dds::core::status::StatusMask& mask
			)
	{
		try {
			dds::core::cond::WaitSet wait_set;
			wait_set += connection_guard;
			wait_set += incompatible_qos_guard;

			auto active_conditions = wait_set.wait(timeout);

			if(active_conditions.size() == 0) {
				// if no specified conditions are active, then the only thing that could have happened is a timeout
				reset(dds_reader);
				return Smart::StatusCode::SMART_SERVICEUNAVAILABLE;
			} else {
				// check the active conditions
				for(auto condition: active_conditions) {
					if(condition == connection_guard) {
						// now we reset the listener pointer
						dds_reader.listener(the_listener, mask);
						return Smart::StatusCode::SMART_OK;
					} else if(condition == incompatible_qos_guard) {
						reset(dds_reader);
						return Smart::StatusCode::SMART_INCOMPATIBLESERVICE;
					}
				}
			}
		} catch(dds::core::Error &error) {
			std::cerr << error.what() << std::endl;
		}
		// make sure the reader is in a consistent disconnected state at the end event in presence of errors
		reset(dds_reader);
		return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
	}

	void DDSReaderConnector::reset(DynamicDataReader &dds_reader) {
		if(!dds_reader.is_nil()) {
			dds_reader.listener(NULL, dds::core::status::StatusMask::none());
			dds_reader = nullptr;
		}
	}
} /* namespace SmartDDS */
