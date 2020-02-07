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

#ifndef RTIDDSSMARTSOFT_SENDSERVERPATTERN_H_
#define RTIDDSSMARTSOFT_SENDSERVERPATTERN_H_

#include <mutex>

#include "RTI-DDS-SmartSoft/Component.h"
#include "RTI-DDS-SmartSoft/SendPatternQoS.h"
#include "RTI-DDS-SmartSoft/DDSReaderConnector.h"

#include <smartISendServerPattern_T.h>

namespace SmartDDS {

template <class DataType>
class SendServerPattern
:	public Smart::ISendServerPattern<DataType>
,	public DynamicDataReaderListener
{
public:
	using ISendServerBase = Smart::ISendServerPattern<DataType>;
	using typename ISendServerBase::ISendServerHandlerPtr;

	SendServerPattern(Component* component, const std::string& serviceName, ISendServerHandlerPtr handler = nullptr)
	:	ISendServerBase(component, serviceName, handler)
	,	component(component)
	,	dds_topic(nullptr)
	,	dds_reader(nullptr)
	,	dds_reader_connector(component, SendPatternQoS::getTopicQoS())
	{
		auto topicName = component->getName()+"::"+serviceName;
		auto dds_dynamic_type = dds_type<DataType>();

		dds_topic = component->DDS().findOrCreateTopic(topicName, dds_dynamic_type);
		dds_reader = dds_reader_connector.create_new_reader(dds_topic, this);
	}
	virtual ~SendServerPattern()
	{
		this->serverInitiatedDisconnect();
	}

private:
	Component* component;
	DynamicDataTopic dds_topic;
	DynamicDataReader dds_reader;
	DDSReaderConnector dds_reader_connector;

	/** implements server-initiated-disconnect (SID)
	 *
	 *	The server-initiated-disconnect is specific to a certain server implementation.
	 *	Each server should be able triggering disconnecting all currently connected
	 *	clients in case e.g. the component of that server is about to shutdown.
	 *	Disconnecting clients before that ensures that the clients remain in a defined
	 *	state (namely disconnected) after the server is gone.
	 */
	virtual void serverInitiatedDisconnect() override
	{
		dds_reader_connector.reset(dds_reader);
		component->DDS().resetTopic(dds_topic);
	}

	virtual void on_data_available(dds::sub::DataReader<dds::core::xtypes::DynamicData>& reader) override
	{
		if(this->is_shutting_down())
		    return;

		// we consume the samples, so the internal buffer gets freed
		auto samples = reader.take();
		for(const auto& sample: samples) {
			if(sample.info().valid()) {
				DataType input;
				convert(sample.data(), input);

				// propagate the actual handling to the registered handler
				ISendServerBase::handleSend(input);

				// check if server has been commanded to shutdown in the meantime, and if so, stop the loop
				if(this->is_shutting_down())
					break;
			}
		}
	}
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_SENDSERVERPATTERN_H_ */
