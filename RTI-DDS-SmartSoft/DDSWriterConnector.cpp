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

#include "RTI-DDS-SmartSoft/DDSWriterConnector.h"

namespace SmartDDS {

    void DDSWriterConnector::on_publication_matched(DynamicDataWriter &writer, const PublicationMatchedStatus &status)
    {
    	if(status.current_count() > 0) {
    		connection_guard.trigger_value(true);
    	}
    }

    DDSWriterConnector::DDSWriterConnector(Component* component, const dds::topic::qos::TopicQos &topic_qos)
	:	component(component)
	,	topic_qos(topic_qos)
	{  }

	void DDSWriterConnector::reset(DynamicDataWriter &dds_writer) const {
		if(!dds_writer.is_nil()) {
			dds_writer.listener(NULL, dds::core::status::StatusMask::none());
			dds_writer = nullptr;
		}
	}

	DynamicDataWriter DDSWriterConnector::create_new_writer(
			const DynamicDataTopic &dds_topic,
			DynamicDataWriterListener* the_listener,
			const dds::core::status::StatusMask& mask) const
	{
		try {
			auto domainParticipant = component->DDS().getDomainParticipant();

			auto writer_qos = dds::core::QosProvider::Default().datawriter_qos();
			writer_qos = topic_qos;

			// If you are using RTI Connext DDS version 6.0.0, please comment in the following two lines of code
			// to workaround a known bug in version 6.0.0, see:
			// https://community.rti.com/forum-topic/segmentation-fault-when-using-contentfilteredtopic-dynamicdata-0
			// Basically, this will deactivate the internal server-side filtering (as a workaround for the bug).
			// This bug has been fixed since version 6.0.1, so if you are using this version or later,
			// you don't need to deactivate server-side filtering anymore (which improves overall performance).
//			rti::core::policy::DataWriterResourceLimits resource_limits;
//			writer_qos << resource_limits.max_remote_reader_filters(0);

			rti::core::policy::Property qos_property;
			writer_qos << qos_property.set({"dds.data_writer.history.memory_manager.fast_pool.pool_buffer_max_size", "32768"});

			return DynamicDataWriter(dds::pub::Publisher(domainParticipant), dds_topic, writer_qos, the_listener, mask);
		} catch(dds::core::Error &error) {
			std::cerr << error.what() << std::endl;
		}
		return DynamicDataWriter(nullptr);
	}

	Smart::StatusCode DDSWriterConnector::reconnect(
			DynamicDataWriter & dds_writer,
			const DynamicDataTopic & dds_topic,
			const Smart::Duration & timeout,
			DynamicDataWriterListener* the_listener,
			const dds::core::status::StatusMask& mask)
	{
		try {
			// reset the connection guard
			connection_guard.trigger_value(false);

			// first we reset the writer with the new attributes including "this" as the listener pointer (see last parameter)
			dds_writer = create_new_writer(dds_topic, this);
			if(dds_writer.is_nil()) {
				return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
			}

			dds::core::cond::WaitSet wait_set;
			wait_set += connection_guard;
			auto active_conditions = wait_set.wait(timeout);

			if(active_conditions.size() == 0) {
				// if no specified conditions are active, then the only thing that could have happened is a timeout
				reset(dds_writer);
				return Smart::StatusCode::SMART_SERVICEUNAVAILABLE;
			} else {
				// check the active conditions
				for(auto condition: active_conditions) {
					if(condition == connection_guard) {
						// now we reset the listener pointer
						dds_writer.listener(the_listener, mask);
						return Smart::StatusCode::SMART_OK;
					}
				}
			}
		} catch(dds::core::Error &error) {
			std::cerr << error.what() << std::endl;
		}
		// make sure the reader is in a consistent disconnected state at the end event in presence of errors
		reset(dds_writer);
		return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
	}

} /* namespace SmartDDS */
