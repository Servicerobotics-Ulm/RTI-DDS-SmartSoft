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

#ifndef RTIDDSSMARTSOFT_PUSHCLIENTPATTERN_H_
#define RTIDDSSMARTSOFT_PUSHCLIENTPATTERN_H_

#include "RtiDdsSmartSoft/Component.h"
#include "RtiDdsSmartSoft/PushPatternQoS.h"
#include "RtiDdsSmartSoft/DDSReaderConnector.h"

#include <smartIPushClientPattern_T.h>

#include <mutex>

namespace SmartDDS {

template <class DataType>
class PushClientPattern
:	public Smart::IPushClientPattern<DataType>
,	public DynamicDataReaderListener
{
private:
	Component *component;

	std::recursive_mutex connection_mutex;

	dds::topic::Topic<dds::core::xtypes::DynamicData> dds_parent_topic;

	// this helper allows checking if a remote end-point actually responds during a connection phase (see connect(...) method)
	DDSReaderConnector dds_reader_connector;

	DynamicDataFilteredTopic dds_subscription_topic;
	DynamicDataReader dds_subscription_reader;

	dds::sub::cond::ReadCondition new_data_guard;
	dds::sub::status::DataState default_read_state;

	// these three guard conditions allow managing the pattern's internal states (i.e. connected, subscribed, blocking)
	// please note, only if all three guards have the "false" value, only then a blocking getUpdateWait is allowed
	dds::core::cond::GuardCondition disconnected_guard;
	dds::core::cond::GuardCondition unsubscribed_guard;
	dds::core::cond::GuardCondition nonblocking_guard;

    void on_liveliness_changed(dds::sub::DataReader<dds::core::xtypes::DynamicData>&,
       const dds::core::status::LivelinessChangedStatus &status)
    {
    	if(status.alive_count() == 0) {
    		disconnected_guard.trigger_value(true);
    	} else if(status.alive_count() > 0) {
    		disconnected_guard.trigger_value(false);
    	}
    }
    void on_data_available(dds::sub::DataReader<dds::core::xtypes::DynamicData> &reader)
    {
		if(this->is_shutting_down() || disconnected_guard.trigger_value())
			return;

		auto samples = reader.read();
		for(auto sample: samples) {
			if(sample.info().valid()) {
				DataType input;
				convert(sample.data(), input);
				this->notify_input(input);
			}
		}
    }

public:
    /** Constructor (not wired with service provider and not exposed as port).
     *  connect() / disconnect() can always be used to change
     *  the status of the instance. Instance is not connected to a service provider
     *  and is not exposed as port wireable from outside the component.
     *
     * @param component  the management class of the component
     */
	PushClientPattern(Component* component)
	:	Smart::IPushClientPattern<DataType>(component)
	,	component(component)
	,	dds_parent_topic(nullptr)
	,	dds_reader_connector(component, PushPatternQoS::getTopicQoS())
	,	dds_subscription_topic(nullptr)
	,	dds_subscription_reader(nullptr)
	,	new_data_guard(nullptr)
	,	default_read_state()
	{
		// by default, the client initializes in the disconnected state
		disconnected_guard.trigger_value(true);
		unsubscribed_guard.trigger_value(true);
		// initialize the default read state to react to not-yet read samples
		default_read_state << dds::sub::status::SampleState::not_read();
	}

    /** Connection Constructor (implicitly wiring with specified service provider).
     *  Connects to the denoted service and blocks until the connection
     *  has been established. Blocks infinitely if denoted service becomes
     *  unavailable since constructor performs retries. Blocking is useful to
     *  simplify startup of components which have mutual dependencies.
     *  connect() / disconnect() can always be used to change
     *  the status of the instance.
     *
     * @param component  the management class of the component
     * @param server     name of the server (i.e. the component-name to connect to)
     * @param service    name of the service (i.e. the port-name of the component to connect to)
     */
	PushClientPattern(Component* component, const std::string& server, const std::string& service)
	:	Smart::IPushClientPattern<DataType>(component, server, service)
	,	component(component)
	,	dds_parent_topic(nullptr)
	,	dds_reader_connector(component, PushPatternQoS::getTopicQoS())
	,	dds_subscription_topic(nullptr)
	,	dds_subscription_reader(nullptr)
	,	new_data_guard(nullptr)
	,	default_read_state()
	{
		// by default, the client initializes in the disconnected state
		disconnected_guard.trigger_value(true);
		unsubscribed_guard.trigger_value(true);
		// initialize the default read state to react to not-yet read samples
		default_read_state << dds::sub::status::SampleState::not_read();
		// this constructor performs an implicit connection
		this->connect(server, service);
	}

    /** Destructor.
     *  The destructor calls disconnect() and therefore properly cleans up
     *  every pending data reception and removes the instance from the set of wireable ports.
     */
    virtual ~PushClientPattern() {
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
			dds_parent_topic = component->DDS().findOrCreateTopic(topicName, dynamic_type);

			ConnectionId empty_id;
			dds_subscription_topic = component->DDS().findOrCreateClientFilteredTopic(dds_parent_topic, empty_id);

			auto timeout = std::chrono::seconds(1);
			auto connection_status = dds_reader_connector.reconnect(dds_subscription_reader, dds_subscription_topic, timeout, this);
			if(connection_status != Smart::StatusCode::SMART_OK) {
				this->disconnect();
				return connection_status;
			} else {
				// connection is now established
				disconnected_guard.trigger_value(false);
				return connection_status;
			}
    	} catch (dds::core::Error &err) {
    		std::cerr << err.what() << std::endl;
    		this->disconnect();
    		return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
    	}
    	// something unexpected went wrong
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
		// first thing is to change the disconnected guard to true so no blocking calls will be used from here on
		disconnected_guard.trigger_value(true);

		std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);

		// by default we also unsubscribe
		this->unsubscribe();

		dds_reader_connector.reset(dds_subscription_reader);
		component->DDS().resetFilteredTopic(dds_subscription_topic);
		component->DDS().resetTopic(dds_parent_topic);

		// make sure the guard has not been reactivated in the meantime
		disconnected_guard.trigger_value(true);

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

    /** Subscribe at the server to periodically get every n-th update. A
     *  newly subscribed client gets the next available new data and is
     *  then updated with regard to its individual prescale-factor.
     *  If the prescale factor is omitted then every update will be received.
     *
     *  @param prescale  whole-numbered value to set the update rate to
     *                   every n-th value (must be greater than 0)
     *
     *  @return status code
     *    - SMART_OK                  : everything is ok and client is subscribed
     *    - SMART_DISCONNECTED        : client is not connected to a server and can therefore
     *                                  not subscribe for updates, not subscribed
     *    - SMART_ERROR_COMMUNICATION : communication problems, not subscribed
     *    - SMART_ERROR               : something went wrong, not subscribed
     */
    virtual Smart::StatusCode subscribe(const unsigned int &prescale = 1) override
    {
    	std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);

    	if(disconnected_guard.trigger_value() == true)
    	    return Smart::StatusCode::SMART_DISCONNECTED;

    	if(prescale < 1) {
    		return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
    	}

    	try {
			if(!dds_subscription_reader.is_nil()) {
				// we temporarily deactivate the connection listener as we will shortly reconnect the reader
				dds_subscription_reader.listener(NULL, dds::core::status::StatusMask::none());
			}

			// create prescale filter using the ID of the pre-connected dds_subscription_reader
			ConnectionId subscriber_id(dds_subscription_reader);
			dds_subscription_topic = component->DDS().findOrCreateClientFilteredTopic(dds_parent_topic, subscriber_id, std::to_string(prescale));

			auto timeout = std::chrono::seconds(1);
			auto connection_status = dds_reader_connector.reconnect(dds_subscription_reader, dds_subscription_topic, timeout, this);
			if(connection_status != Smart::StatusCode::SMART_OK) {
				this->unsubscribe();
				return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
			} else {
				// re-attach the read condition to the new reader
				new_data_guard = dds::sub::cond::ReadCondition(dds_subscription_reader, default_read_state);
				unsubscribed_guard.trigger_value(false);
			}
			return connection_status;
    	} catch (dds::core::Error &err) {
    		std::cerr << err.what() << std::endl;
    		return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
    	}

    	return Smart::StatusCode::SMART_ERROR;
    }

    /** Unsubscribe to get no more updates. All blocking calls are aborted with the appropriate
     *  status and yet received and still buffered data is deleted to avoid returning old data.
     *
     *  @return status code
     *    - SMART_OK                  : everything is ok and client is now unsubscribed or
     *                                  has already been unsubscribed
     *    - SMART_ERROR_COMMUNICATION : communication problems, not unsubscribed
     *    - SMART_ERROR               : something went wrong, not unsubscribed
     *
     * (can not return SMART_DISCONNECTED since then client is for sure also unsubscribed
     *  which results in SMART_OK)
     */
    virtual Smart::StatusCode unsubscribe() override
    {
    	std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);

    	if(disconnected_guard.trigger_value() == true)
    	    return Smart::StatusCode::SMART_OK;

		unsubscribed_guard.trigger_value(true);

		try {
			new_data_guard = nullptr;

			if(!dds_subscription_reader.is_nil()) {
				dds_subscription_reader.listener(NULL, dds::core::status::StatusMask::none());
			}

			ConnectionId empty_id;
			dds_subscription_topic = component->DDS().findOrCreateClientFilteredTopic(dds_parent_topic, empty_id);
			auto timeout = std::chrono::seconds(1);
			auto connection_status = dds_reader_connector.reconnect(dds_subscription_reader, dds_subscription_topic, timeout, this);
		} catch (dds::core::Error &err) {
			std::cerr << err.what() << std::endl;
			return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
		}

    	return Smart::StatusCode::SMART_OK;
    }

    /** Non-blocking call to immediately return the latest available
     *  data buffered at the client side from the most recent update.
     *
     *  No data is returned as long as no update is received since
     *  subscription. To avoid returning old data, no data is
     *  returned after the client is unsubscribed or when the
     *  server is not active.
     *
     * @param d is set to the newest currently available data
     *
     * @return status code
     *   - SMART_OK                  : everything ok and latest data since client got subscribed
     *                                 is returned.
     *   - SMART_NODATA              : client has not yet received an update since subscription and
     *                                 therefore no data is available and no data is returned.
     *   - SMART_UNSUBSCRIBED        : no data available since client is not subscribed and can
     *                                 therefore not receive updates. Method does not return old data from
     *                                 last subscription since these might be based on too old parameter
     *                                 settings. To get data one has to be subscribed.
     *   - SMART_DISCONNECTED        : no data returned since client is even not connected to a server.
     *   - SMART_ERROR               : something went wrong
     */
    virtual Smart::StatusCode getUpdate(DataType& d) override
    {
    	std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);

    	if(disconnected_guard.trigger_value() == true)
    		return Smart::StatusCode::SMART_DISCONNECTED;
    	if(unsubscribed_guard.trigger_value() == true)
    		return Smart::StatusCode::SMART_UNSUBSCRIBED;

    	try {
        	// read the current samples (data will not be consumed)
			auto samples = dds_subscription_reader.read();
			if(samples.length() == 0)
				return Smart::StatusCode::SMART_NODATA;

			for(const auto& sample: samples) {
				if(sample.info().valid()) {
					// convert and copy the DDS sample into the provided communication object reference
					// we take only the first valid sample as we can assume there only is one sample
					convert(sample.data(), d);
					return Smart::StatusCode::SMART_OK;
				}
			}
    	} catch(dds::core::Error &error) {
    		std::cerr << error.what() << std::endl;
    		return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
    	}
    	// something unexpected went wrong
    	return Smart::StatusCode::SMART_ERROR;
    }

    /** Blocking call which waits until the next update is received.
     *
     *  Blocking is aborted with the appropriate status if either the
     *  server gets deactivated, the client gets unsubscribed or disconnected
     *  or if blocking is not allowed any more at the client.
     *
     *  @param d is set to the newest currently available data
     *  @param timeout is the timeout time to block the method maximally (default value max blocks infinitely)
     *
     *  @return status code
     *   - SMART_OK                  : everything is ok and just received data is returned.
     *   - SMART_CANCELLED           : blocking is not allowed or is not allowed anymore. Waiting for the
     *                                 next update is aborted and no valid data is returned.
     *   - SMART_UNSUBSCRIBED        : returns immediately without data if the client is not subscribed.
     *   - SMART_DISCONNECTED        : returns immediately without data since client is even not connected
     *                                 to a server.
     *   - SMART_ERROR               : something went completely wrong and no valid data returned.
     */
    virtual Smart::StatusCode getUpdateWait(DataType& d, const Smart::Duration &timeout=Smart::Duration::max()) override
    {
    	std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);

    	// don't try creating a wait-set if client is disconnected
    	if(disconnected_guard.trigger_value() == true)
    		return Smart::StatusCode::SMART_DISCONNECTED;
    	if(unsubscribed_guard.trigger_value() == true)
    	    return Smart::StatusCode::SMART_UNSUBSCRIBED;

		try {
			// because the getUpdateWait method can be called multiple times in parallel
			// from different threads, while a WaitSet can only be used from a single thread,
			// therefore we need to create a new WaitSet for each separate call of getUpdateWait
			dds::core::cond::WaitSet wait_set;
			// new-data condition is triggered each time a new value becomes available
			wait_set += new_data_guard;
			wait_set += disconnected_guard;
			wait_set += unsubscribed_guard;
			wait_set += nonblocking_guard;

			// unlock the connection mutex so the pattern remains responsive
			connection_lock.unlock();

			// blocking wait until one of the specified conditions becomes true (or a timeout occurs)
			auto active_conditions = wait_set.wait(timeout);

			if(active_conditions.size() == 0) {
				// if no specified conditions are active, then the only thing that could have happened is a timeout
				return Smart::StatusCode::SMART_TIMEOUT;
			}

			// check the active conditions
			for(auto cond: active_conditions) {
				// we only need to check the non-blocking guard explicitly here,
				// all the other cases are checked within the getUpdate(...) call below
				if(cond == nonblocking_guard)
					return Smart::StatusCode::SMART_CANCELLED;
			}
		} catch(dds::core::Error &error) {
			std::cerr << error.what() << std::endl;
			return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
		}
    	// propagate the actual reading of data to the non-blocking method getUpdate(...),
		// which is safe to use now as we have finished waiting and have checked the non-blocking case
		return this->getUpdate(d);
    }
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_PUSHCLIENTPATTERN_H_ */
