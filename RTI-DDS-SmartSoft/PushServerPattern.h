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

#ifndef RTIDDSSMARTSOFT_PUSHSERVERPATTERN_H_
#define RTIDDSSMARTSOFT_PUSHSERVERPATTERN_H_

#include <mutex>

#include "RtiDdsSmartSoft/Component.h"
#include "RtiDdsSmartSoft/PushPatternQoS.h"

#include "RtiDdsSmartSoft/DDSAliases.h"
#include "RtiDdsSmartSoft/DDSWriterConnector.h"

#include <smartIPushServerPattern_T.h>

namespace SmartDDS {

template <class DataType>
class PushServerPattern : public Smart::IPushServerPattern<DataType> {
private:
	Component* component;

	std::mutex server_mutex;

	DDSWriterConnector dds_writer_connector;
	dds::topic::Topic<dds::core::xtypes::DynamicData> dds_topic;
	dds::pub::DataWriter<dds::core::xtypes::DynamicData> dds_writer;


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
		std::unique_lock<std::mutex> server_lock(server_mutex);
		dds_writer_connector.reset(dds_writer);
		component->DDS().resetTopic(dds_topic);
	}

public:
	PushServerPattern(Component* component, const std::string& serviceName, const Smart::Duration &cycleTime = Smart::Duration::zero())
	:	Smart::IPushServerPattern<DataType>(component, serviceName)
	,	component(component)
	,	dds_writer_connector(component, PushPatternQoS::getTopicQoS())
	,	dds_topic(nullptr)
	,	dds_writer(nullptr)
	{
		auto topicName = component->getName()+"::"+serviceName;
		auto dds_dynamic_type = dds_type<DataType>();
		dds_topic = component->DDS().findOrCreateTopic(topicName, dds_dynamic_type);
		dds_writer = dds_writer_connector.create_new_writer(dds_topic);
	}

	virtual ~PushServerPattern()
	{
		this->serverInitiatedDisconnect();
	}

    /** Provide new data which is sent to all subscribed clients
     *  taking into account their individual prescale factors.
     *  Prescale factors are always whole-numbered multiples of the server
     *  update intervals.
     *
     *  (Individual update interval counters are incremented each
     *   time this member function is called irrespectively of the
     *   elapsed time. One should use the time triggered handler to
     *   call the put() member function with the appropriate timing.)
     *
     *  @param data contains the newly acquired data to be sent as update.
     *
     *  @return status code
     *    - SMART_OK                  : everything is ok
     *    - SMART_CANCELLED           : server is in the process of shutting down
     *    - SMART_ERROR_COMMUNICATION : communication problems caused by at least
     *                                  one client. The other clients are updated
     *                                  correctly.
     *    - SMART_ERROR               : something went completely wrong with at least one
     *                                  client. Some clients might still been
     *                                  updated correctly.
     */
    virtual Smart::StatusCode put(const DataType& data) override
    {
    	std::unique_lock<std::mutex> server_lock(server_mutex);

   		try {
   			// the writer might be in the process of shutting down
   			if(dds_writer.is_nil())
   				return Smart::StatusCode::SMART_CANCELLED;

   			// write the serialized data
   			dds_writer.write(serialize(data));

			// as long as no exceptions are thrown we assume that the communication was successful
			return Smart::StatusCode::SMART_OK;
		} catch (std::exception &ex) {
			std::cerr << ex.what() << std::endl;
			return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
		}
    }
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_PUSHSERVERPATTERN_H_ */
