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

#ifndef RTIDDSSMARTSOFT_SENDCLIENTPATTERN_H_
#define RTIDDSSMARTSOFT_SENDCLIENTPATTERN_H_

#include "RTI-DDS-SmartSoft/Component.h"
#include "RTI-DDS-SmartSoft/SendPatternQoS.h"

#include "RTI-DDS-SmartSoft/DDSAliases.h"
#include "RTI-DDS-SmartSoft/DDSWriterConnector.h"

#include <smartISendClientPattern_T.h>

#include <mutex>

namespace SmartDDS {

template <class DataType>
class SendClientPattern
:	public Smart::ISendClientPattern<DataType>
,	public DynamicDataWriterListener
{
private:
	Component *component;

	DynamicDataTopic dds_topic;
	DynamicDataWriter dds_writer;

	DDSWriterConnector dds_writer_connector;

	std::recursive_mutex connection_mutex;
	dds::core::cond::GuardCondition connection_guard;
	dds::core::cond::GuardCondition nonblocking_guard;

    virtual void on_publication_matched(DynamicDataWriter&, const PublicationMatchedStatus &status) override
    {
    	if(status.current_count() == 0) {
    		connection_guard.trigger_value(false);
    	} else if(status.current_count() > 0) {
    		connection_guard.trigger_value(true);
    	}
    }
public:
	SendClientPattern(Component* component)
	:	Smart::ISendClientPattern<DataType>(component)
	,	component(component)
	,	dds_topic(nullptr)
	,	dds_writer(nullptr)
	,	dds_writer_connector(component, SendPatternQoS::getTopicQoS())
	{   }
	SendClientPattern(Component* component, const std::string& server, const std::string& service)
	:	Smart::ISendClientPattern<DataType>(component, server, service)
	,	component(component)
	,	dds_topic(nullptr)
	,	dds_writer(nullptr)
	,	dds_writer_connector(component, SendPatternQoS::getTopicQoS())
	{
		// this constructor performs an implicit connection
		this->connect(server, service);
	}

	virtual ~SendClientPattern()
	{
		this->disconnect();
	}

    /** Connect this service requestor to the denoted service provider. An
     *  already established connection is first disconnected. See disconnect()
     *
     *  It is no problem to change the connection to a service provider at any
     *  point in time irrespective of any other calls.
     *
     * @param server     name of the server (i.e. the component-name to connect to)
     * @param service    name of the service (i.e. the port-name of the component to connect to)
     *
     *  @return status code
     *   - SMART_OK                  : everything is OK and connected to the specified service.
     *   - SMART_SERVICEUNAVAILABLE  : the specified service is currently not available and the
     *                                 requested connection can not be established. Service
     *                                 requestor is now not connected to any service provider.
     *   - SMART_INCOMPATIBLESERVICE : the specified service provider is not compatible (wrong communication
     *                                 pattern or wrong communication objects) to this service requestor and
     *                                 can therefore not be connected. Service requestor is now not connected
     *                                 to any service provider.
     *   - SMART_ERROR_COMMUNICATION : communication problems, service requestor is now not connected to any
     *                                 service provider.
     *   - SMART_ERROR               : something went wrong, service requestor is now not connected to any
     *                                 service provider.
     */
    virtual Smart::StatusCode connect(const std::string& server, const std::string& service) override
    {
    	std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);

    	// we by default disconnect the previous connection
    	this->disconnect();

    	this->connectionServerName = server;
    	this->connectionServiceName = service;

    	try {
			// the topic-name is constructed from the component-instance-name and a server-port-name
			std::string topicName = server+"::"+service;
			// the dynamic type is determined from an externally implemented template method dds_type()
			auto dynamic_type = dds_type<DataType>();
			dds_topic = component->DDS().findOrCreateTopic(topicName, dynamic_type);

			auto timeout = std::chrono::seconds(1);
			auto connection_status = dds_writer_connector.reconnect(dds_writer, dds_topic, timeout, this);
			if(connection_status != Smart::StatusCode::SMART_OK) {
				this->disconnect();
			} else {
				connection_guard.trigger_value(true);
			}
			return connection_status;
		} catch(dds::core::Error &error) {
			std::cerr << error.what() << std::endl;
			// we need to make sure that we end up in a defined disconnected/clean state
			this->disconnect();
			return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
		}

		return Smart::StatusCode::SMART_ERROR;
    }

    /** Disconnect the service requestor from the service provider.
     *
     *  It is no problem to change the connection to a service provider at any
     *  point in time irrespective of any other calls.
     *  @return status code
     *   - SMART_OK                  : everything is OK and service requestor is disconnected from
     *                                 the service provider.
     *   - SMART_ERROR_COMMUNICATION : something went wrong at the level of the intercomponent
     *                                 communication. At least the service requestor is in the
     *                                 disconnected state irrespective of the service provider
     *                                 side clean up procedures.
     *   - SMART_ERROR               : something went wrong. Again at least the service requestor
     *                                 is in the disconnected state.
     */
    virtual Smart::StatusCode disconnect() override
    {
    	std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);

    	dds_writer_connector.reset(dds_writer);
    	component->DDS().resetTopic(dds_topic);

		connection_guard.trigger_value(false);

    	return Smart::StatusCode::SMART_OK;
    }

    /** Allow or abort and reject blocking calls.
     *
     *  If blocking is set to false all blocking calls return with SMART_CANCELLED. This can be
     *  used to abort blocking calls.
     *
     *  @param blocking  true/false
     *
     *  @return status code
     *   - SMART_OK                  : new mode set
     *   - SMART_ERROR               : something went wrong
     */
    virtual Smart::StatusCode blocking(const bool blocking) override
    {
    	std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);
    	this->is_blocking = blocking;
    	nonblocking_guard.trigger_value(blocking);
    	return Smart::StatusCode::SMART_OK;
    }

    /** Perform a one-way communication. Appropriate status codes make
     *  sure that the information has been transferred.
     *
     *  @param data the object to be sent (Communication Object)
     *
     *  @return status code:
     *    - SMART_OK                  : everything is ok and communication object sent to server
     *    - SMART_DISCONNECTED        : the client is disconnected and no send can be made
     *    - SMART_ERROR_COMMUNICATION : communication problems, data not transmitted
     *    - SMART_ERROR               : something went wrong, data not transmitted
     */
    virtual Smart::StatusCode send(const DataType& data) override
    {
    	// do not initiate new communication when shutting down
    	if(connection_guard.trigger_value() == false)
    		return Smart::StatusCode::SMART_DISCONNECTED;

    	std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);

		// as long as no exceptions are thrown we assume that the communication was successful
   		try {
   			// send the data sample
			dds_writer.write(serialize(data));
			return Smart::StatusCode::SMART_OK;
		} catch (std::exception &ex) {
			std::cerr << ex.what() << std::endl;
			return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
		}
    }
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_SENDCLIENTPATTERN_H_ */
