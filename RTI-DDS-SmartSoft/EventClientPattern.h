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

#ifndef RTIDDSSMARTSOFT_EVENTCLIENTPATTERN_H_
#define RTIDDSSMARTSOFT_EVENTCLIENTPATTERN_H_

#include <map>
#include <mutex>
#include <memory>

#include "RTI-DDS-SmartSoft/Component.h"

#include "RTI-DDS-SmartSoft/EventResult.h"
#include "RTI-DDS-SmartSoft/EventPatternQoS.h"
#include "RTI-DDS-SmartSoft/EventActivationDecorator.h"

#include "RTI-DDS-SmartSoft/DDSReaderConnector.h"
#include "RTI-DDS-SmartSoft/DDSWriterConnector.h"

#include <smartIEventClientPattern_T.h>

namespace SmartDDS {

template<class ActivationType, class EventType>
class EventClientPattern;

template <class ActivationType, class EventType>
class EventHandler : public Smart::IEventHandler<EventType>
{
public:
	EventHandler(EventClientPattern<ActivationType,EventType> *client)
	:	Smart::IEventHandler<EventType>(client)
	{  }
	// implement the abstract methods from the base interface in derived classes
};

template<class ActivationType, class EventType>
class EventClientPattern
:	public Smart::IEventClientPattern<ActivationType,EventType>
,	public dds::sub::NoOpDataReaderListener<dds::core::xtypes::DynamicData>
{
private:

	Component *component;

	std::recursive_mutex client_mutex;
	std::map<CorrelationId, std::shared_ptr<EventResult<EventType>>> event_cache;

	EventActivationDecorator activation_decorator;

	// this helpers allow checking if a remote end-point actually responds during a connection phase (see connect(...) method)
	DDSWriterConnector dds_writer_connector;
	DDSReaderConnector dds_reader_connector;

	dds::topic::Topic<dds::core::xtypes::DynamicData> dds_activation_topic;
	dds::pub::DataWriter<dds::core::xtypes::DynamicData> dds_activation_writer;

	dds::topic::Topic<dds::core::xtypes::DynamicData> dds_event_topic;
	dds::topic::ContentFilteredTopic<dds::core::xtypes::DynamicData> dds_filtered_event_topic;
	dds::sub::DataReader<dds::core::xtypes::DynamicData> dds_filtered_event_reader;

	dds::core::cond::GuardCondition disconnected_guard;
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
    	// do not grab the mutex if client is in the process of shutting down or disconnecting
		if(this->is_shutting_down() || disconnected_guard.trigger_value() == true)
			return;

		std::unique_lock<std::recursive_mutex> client_lock(client_mutex);

		// consume all incoming events
		auto events = reader.take();
		for(auto event: events) {
			if(event.info().valid()) {
				// create event input object
				Smart::EventInputType<EventType> input;
				CorrelationId related_id = CorrelationId::createRelatedId(event.info());
				input.event_id = std::make_shared<CorrelationId>(related_id);

				convert(event.data(), input.event);

				// first we first copy the new event into the internal cache
				auto event_it = event_cache.find(related_id);
				if(event_it != event_cache.end()) {
					// this call overrides the internal event copy for the given ID
					event_it->second->setNewEvent(input.event);
				}

				// now we notify all potentially registered input handlers
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
	EventClientPattern(Component* component)
	:	Smart::IEventClientPattern<ActivationType, EventType>(component)
	,	component(component)
	,	activation_decorator(dds_type<ActivationType>())
	,	dds_writer_connector(component, EventPatternQoS::getActivationTopicQoS())
	,	dds_reader_connector(component, EventPatternQoS::getEventTopicQoS())
	,	dds_activation_topic(nullptr)
	,	dds_activation_writer(nullptr)
	,	dds_event_topic(nullptr)
	,	dds_filtered_event_topic(nullptr)
	,	dds_filtered_event_reader(nullptr)
	{
		// by default, the client initializes in the disconnected state
		disconnected_guard.trigger_value(true);
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
	EventClientPattern(Component* component, const std::string& server, const std::string& service)
	:	Smart::IEventClientPattern<ActivationType, EventType>(component, server, service)
	,	component(component)
	,	activation_decorator(dds_type<ActivationType>())
	,	dds_writer_connector(component, EventPatternQoS::getActivationTopicQoS())
	,	dds_reader_connector(component, EventPatternQoS::getEventTopicQoS())
	,	dds_activation_topic(nullptr)
	,	dds_activation_writer(nullptr)
	,	dds_event_topic(nullptr)
	,	dds_filtered_event_topic(nullptr)
	,	dds_filtered_event_reader(nullptr)
	{
		// by default, the client initializes in the disconnected state
		disconnected_guard.trigger_value(true);

		this->connect(server, service);
	}

    /** Destructor.
     *  The destructor calls disconnect() and therefore properly cleans up
     *  every pending data reception and removes the instance from the set of wireable ports.
     */
	virtual ~EventClientPattern()
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
    	std::unique_lock<std::recursive_mutex> client_lock(client_mutex);

    	// we by default disconnect the previous connection
    	this->disconnect();

    	this->connectionServerName = server;
    	this->connectionServiceName = service;

		auto activationTopicName = server+"::"+service+"::EventActivationTopic";
		auto eventTopicName = server+"::"+service+"::EventTopic";

		// the dynamic DDS types are determined using external template methods
		auto dds_activation_type = activation_decorator.getDecoratedDDSType();
		auto dds_event_type = dds_type<EventType>();

		try {
			// if the related event server is in the same component, then the topic is already defined and we can simply reuse it
			dds_activation_topic = component->DDS().findOrCreateTopic(activationTopicName, dds_activation_type);
			dds_event_topic = component->DDS().findOrCreateTopic(eventTopicName, dds_event_type);

			auto timeout = std::chrono::seconds(1);
			auto connection_status = dds_writer_connector.reconnect(dds_activation_writer, dds_activation_topic, timeout);

			if(connection_status != Smart::StatusCode::SMART_OK) {
				this->disconnect();
				return connection_status;
			}

			// initiate the connection ID from the activation writer
			ConnectionId requester_id(dds_activation_writer);

			// create a content filtered topic
			dds_filtered_event_topic = component->DDS().findOrCreateClientFilteredTopic(dds_event_topic, requester_id);

			// reconnect the reader to the filtered topic
			connection_status = dds_reader_connector.reconnect(dds_filtered_event_reader, dds_filtered_event_topic, timeout, this);
			if(connection_status != Smart::StatusCode::SMART_OK) {
				this->disconnect();
			} else {
				disconnected_guard.trigger_value(false);
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
    	// first thing is to change the disconnected guard to true so no blocking calls will be used from here on
    	disconnected_guard.trigger_value(true);

		std::unique_lock<std::recursive_mutex> client_lock(client_mutex);

		for(auto event: event_cache) {
			event.second->deactivateEvent();
		}
		event_cache.clear();

		// release the resource for the request-channel
		dds_writer_connector.reset(dds_activation_writer);
		component->DDS().resetTopic(dds_activation_topic);

		// release the resource for the reply-channel
		dds_reader_connector.reset(dds_filtered_event_reader);
		component->DDS().resetFilteredTopic(dds_filtered_event_topic);
		component->DDS().resetTopic(dds_event_topic);

		// make sure the disconnected guard has not been reset by the on_liveliness_changed upcall unintentionally
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
    	std::unique_lock<std::recursive_mutex> client_lock(client_mutex);
    	this->is_blocking = blocking;
    	nonblocking_guard.trigger_value(blocking);
    	return Smart::StatusCode::SMART_OK;
    }

    /** Activate an event with the provided parameters in either "single" or "continuous" mode.
     *
     *  @param mode        "single" or "continuous" mode
     *  @param parameter   activation parameter class (Communication Object)
     *  @param id          is set to the unique id of the event activation
     *
     *  @return status code
     *    - SMART_OK                  : everything is ok, event is activated and <I>id</I> contains
     *                                  a valid activation identifier.
     *    - SMART_DISCONNECTED        : activation not possible since not connected to a server. No
     *                                  valid activation identifier <I>id</I> returned.
     *    - SMART_ERROR_COMMUNICATION : communication problems, event not activated, <I>id</I> is not
     *                                  a valid activation identifier.
     *    - SMART_ERROR               : something went wrong, event not activated, <I>id</I> is not
     *                                  a valid activation identifier.
     */
    virtual Smart::StatusCode activate(const Smart::EventMode &mode, const ActivationType& parameter, Smart::EventIdPtr& id) override
    {
    	std::unique_lock<std::recursive_mutex> client_lock(client_mutex);

    	if(disconnected_guard.trigger_value() == true)
    		return Smart::StatusCode::SMART_DISCONNECTED;

    	try {
    		EventActivationMode activation_mode(EventActivationMode::ACTIVATE_SINGLE);
    		if(mode == Smart::EventMode::continuous) {
    			activation_mode = EventActivationMode::ACTIVATE_CONTINUOUS;
    		}

    		auto activation_data = activation_decorator.createDecoratedObject(serialize(parameter), activation_mode);

    		// use write-parameters to determine the generated sample ID
    		rti::pub::WriteParams params;
    		params.replace_automatic_values(true);

    		// sends the event activation request (using the extended write method)
    		dds_activation_writer->write(activation_data, params);
    		// get the generated sample ID
    		auto activation_id = params.identity();
    		id = std::make_shared<CorrelationId>(activation_id);

    		// store the event ID within the internal cache for later checking of valid activated event IDs
    		event_cache[activation_id] = std::make_shared<EventResult<EventType>>(mode);
			return Smart::StatusCode::SMART_OK;
    	} catch(std::exception &ex) {
    		std::cerr << ex.what() << std::endl;
    		return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
    	}

    	return Smart::StatusCode::SMART_ERROR;
    }

    /** Deactivate the event with the specified identifier.
     *
     *  An event must always be deactivated, even if it has already
     *  fired in single mode. This is just necessary for cleanup
     *  procedures and provides a uniform user API independently of the
     *  event mode. Calling deactivate() while there are blocking calls
     *  aborts them with the appropriate status code.
     *
     *  @param id of event to be disabled
     *
     *  @return status code
     *    - SMART_OK                  : everything is ok and event is deactivated
     *    - SMART_WRONGID             : there is no active event available with this id
     *    - SMART_ERROR_COMMUNICATION : communication problems, event not deactivated
     *    - SMART_ERROR               : something went wrong, event not deactivated
     *
     * (Hint: can not return SMART_DISCONNECTED since then each event is for sure also
     *        deactivated resulting in SMART_WRONGID)
     */
    virtual Smart::StatusCode deactivate(const Smart::EventIdPtr id) override
    {
    	std::unique_lock<std::recursive_mutex> client_lock(client_mutex);

    	// downcast the abstract EventId pointer to the DDS-specific pointer type
    	auto dds_id = std::dynamic_pointer_cast<CorrelationId>(id);
    	if(!dds_id)
    		return Smart::StatusCode::SMART_WRONGID;
    	auto foundEventIt = event_cache.find(*dds_id);
    	if(foundEventIt == event_cache.end())
    		return Smart::StatusCode::SMART_WRONGID;

    	try {
        	if(disconnected_guard.trigger_value() == false) {
				auto deactivation_object = activation_decorator.createDeactivationObject();

				// use write-parameters to set the related ID
				rti::pub::WriteParams params;
				params.related_sample_identity(*dds_id);

				// sends the event deactivation request (using the extended write method)
				dds_activation_writer->write(deactivation_object, params);
        	}

        	// signal the potentially waiting getEvent/getNextEvent calls to unblock
        	foundEventIt->second->deactivateEvent();

    		// remove event ID from the internal cache (it is save to do so as we are using a shared pointer)
        	event_cache.erase(foundEventIt);
			return Smart::StatusCode::SMART_OK;
    	} catch(std::exception &ex) {
    		std::cerr << ex.what() << std::endl;
    		return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
    	}

    	return Smart::StatusCode::SMART_ERROR;
    }

    /** Check whether event has already fired and return immediately
     *  with status information.
     *
     *  This method does not consume an available event.
     *
     *  @param id of the event activation to be checked
     *
     *  @return status code
     *    - single mode:
     *      - SMART_OK                : event fired already, is still available and can be consumed by
     *                                  calling getEvent(),
     *      - SMART_ACTIVE            : event has not yet fired
     *      - SMART_PASSIVE           : event fired already and is already consumed.
     *      - SMART_WRONGID           : there is no activation available with this <I>id</I>
     *    - continuous mode:
     *      - SMART_OK                : unconsumed event is available. Since events are
     *                                  overwritten this means that at least one new
     *                                  event has been received since the last event
     *                                  consumption.
     *      - SMART_ACTIVE            : currently there is no unconsumed event available.
     *      - SMART_WRONGID           : there is no activation available with this <I>id</I>
     */
    virtual Smart::StatusCode tryEvent(const Smart::EventIdPtr id) override
    {
    	std::unique_lock<std::recursive_mutex> client_lock(client_mutex);

    	// downcast the abstract EventId pointer to the DDS-specific pointer type
    	auto dds_id = std::dynamic_pointer_cast<CorrelationId>(id);
    	if(!dds_id)
    		return Smart::StatusCode::SMART_WRONGID;
    	auto foundEventIt = event_cache.find(*dds_id);
    	if(foundEventIt == event_cache.end())
    		return Smart::StatusCode::SMART_WRONGID;

    	if(foundEventIt->second->hasNewEvent()) {
    		// a new (not yet consumed) event is available
    		return Smart::StatusCode::SMART_OK;
    	} else {
    		if(foundEventIt->second->getEventGuard().trigger_value() == true) {
    			// event has been consumed in the single mode
    			return Smart::StatusCode::SMART_PASSIVE;
    		}
    		// the event has not yet been received or the last event has been consumed
    		return Smart::StatusCode::SMART_ACTIVE;
    	}
    }

    /** Blocking call which waits for the event to fire and then consumes the event.
     *
     *  This method consumes an event. Returns immediately if an unconsumed event is
     *  available. Blocks otherwise till event becomes available. If method is called
     *  concurrently from several threads with the same <I>id</I> and method is blocking,
     *  then every call returns with the same <I>event</I> once the event fired. If there is
     *  however already an unconsumed event available, then only one out of the concurrent
     *  calls consumes the event and the other calls return with appropriate status codes.
     *
     *  @param id of the event activation
     *  @param event is set to the returned event if fired (Communication Object)
     *  @param timeout is the timeout time to block the method maximally (default value zero block infinitelly)
     *
     *  - <b>single mode</b>:
     *      <p>
     *      Since an event in single mode fires only once, return immediately
     *      if the event is already consumed. Also return immediately with an
     *      available and unconsumed event and consume it. Otherwise wait until
     *      the event fires.
     *      </p>
     *      <p>
     *      <b>Returns status code</b>:
     *      </p>
     *      <p>
     *        - SMART_OK            : event fired and event is consumed and returned.
     *        - SMART_PASSIVE       : event fired and got consumed already. Returns immediately without
     *                                valid event since it can not fire again in single mode.
     *        - SMART_CANCELLED     : waiting for the event to fire has been aborted or blocking is not
     *                                not allowed anymore. Therefore no valid <I>event</I> is returned.
     *        - SMART_DISCONNECTED  : client is disconnected or got disconnected while waiting and
     *                                therefore no valid <I>event</I> is returned and the activation
     *                                identifier <I>id</I> is also not valid any longer due to
     *                                automatic deactivation.
     *        - SMART_NOTACTIVATED  : got deactivated while waiting and therefore <I>event</I> not valid and
     *                                also <I>id</I> not valid any longer.
     *        - SMART_WRONGID       : there is no activation available with this <I>id</I> and therefore
     *                                <I>event</I> not valid.
     *      </p>
     *
     *  - <b>continuous mode</b>:
     *     <p>
     *     Returns immediately if an unconsumed event is
     *     available. Returns the newest unconsumed event since
     *     activation. Otherwise waits until the event fires again.
     *     </p>
     *     <p>
     *     <b>Returns status code</b>:
     *     </p>
     *     <p>
     *        - SMART_OK            : unconsumed event is available and event is consumed and returned.
     *                                Due to the overwriting behavior, only the latest event is available.
     *        - SMART_CANCELLED     : blocking is not allowed anymore therefore blocking call has been aborted and
     *                                <I>event</I> is not valid.
     *        - SMART_DISCONNECTED  : got disconnected while waiting and therefore <I>event</I> not valid and
     *                                also <I>id</I> not valid any longer due to automatic deactivation.
     *        - SMART_NOTACTIVATED  : got deactivated while waiting and therefore <I>event</I> not valid and
     *                                also <I>id</I> not valid any longer.
     *        - SMART_WRONGID       : there is no activation available with this <I>id</I> and therefore
     *                                <I>event</I> not valid.
     *     </p>
     */
    virtual Smart::StatusCode getEvent(const Smart::EventIdPtr id, EventType& event, const Smart::Duration &timeout=Smart::Duration::max()) override
    {
    	std::unique_lock<std::recursive_mutex> client_lock(client_mutex);

    	if(disconnected_guard.trigger_value() == true)
    		return Smart::StatusCode::SMART_DISCONNECTED;

    	// downcast the abstract EventId pointer to the DDS-specific pointer type
    	auto dds_id = std::dynamic_pointer_cast<CorrelationId>(id);
    	if(!dds_id)
    		return Smart::StatusCode::SMART_WRONGID;
    	auto foundEventIt = event_cache.find(*dds_id);
    	if(foundEventIt == event_cache.end())
    		return Smart::StatusCode::SMART_WRONGID;

    	try {
    		// this copies the internal shared pointer (which prevents its destruction as long as we have a copy here)
    		auto event_result_ptr = foundEventIt->second;

			dds::core::cond::WaitSet wait_set;
			wait_set += disconnected_guard;
			wait_set += nonblocking_guard;

			// these two guards are specific to the given EventId
			wait_set += event_result_ptr->getDeactivationGuard();
			wait_set += event_result_ptr->getEventGuard();

			// release the lock to ensure that the patterns remains responsive while this method waits
			client_lock.unlock();

			// wait for one of the provided conditions to become true
			auto active_conditions = wait_set.wait(timeout);

			client_lock.lock();

			if(active_conditions.size() == 0) {
				// if no specified conditions are active, then the only thing that could have happened is a timeout
				return Smart::StatusCode::SMART_TIMEOUT;
			}

			// check which condition caused the unblocking and return the related result
			for(auto condition: active_conditions) {
				if(condition == disconnected_guard) {
					// client has disconnected
					return Smart::StatusCode::SMART_DISCONNECTED;
				} else if(condition == nonblocking_guard) {
					// blocking has been set to false
					return Smart::StatusCode::SMART_CANCELLED;
				} else if(condition == event_result_ptr->getDeactivationGuard()) {
					// event has been deactivated
					return Smart::StatusCode::SMART_NOTACTIVATED;
				} else if(condition == event_result_ptr->getEventGuard()) {
					if(event_result_ptr->hasNewEvent()) {
						// consume the event
						event = event_result_ptr->consumeEvent();
						return Smart::StatusCode::SMART_OK;
					} else if(event_result_ptr->isSingleMode()) {
						// event has already been consumed (in single mode, this means SMART_PASSIVE)
						return Smart::StatusCode::SMART_PASSIVE;
					}
				}
			}
		} catch(dds::core::Error &error) {
			std::cerr << error.what() << std::endl;
			return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
		}

    	// something went wrong (this should not happen in normal circumstances)
    	return Smart::StatusCode::SMART_ERROR;
    }

    /** Blocking call which waits for the next event.
     *
     *  This methods waits for the <I>next</I> arriving event to make sure that only events arriving
     *  after entering the method are considered. Method consumes event. An old event that has been
     *  fired is ignored (in contrary to getEvent()). If method is called concurrently from several
     *  threads with the same <I>id</I>, then every call returns with the same <I>event</I> once the
     *  event fired.
     *
     *  @param id of the event activation
     *  @param event is set to the returned event if fired (Communication Object)
     *  @param timeout is the timeout time to block the method maximally (default value zero block infinitelly)
     *
     *  - <b>single mode</b>:
     *    <p>
     *    In single mode one misses the event if it fired before entering this member function.
     *    </p>
     *    <p>
     *    <b>Returns status code</b>:
     *    </p>
     *    <p>
     *      - SMART_OK            : event fired while waiting for the event and event is consumed
     *                              and returned
     *      - SMART_PASSIVE       : event already fired between activation and calling this member
     *                              function and is therefore missed or event has already been
     *                              consumed and can not fire again in single mode. Does not block
     *                              indefinitely and returns no valid <I>event</I>.
     *      - SMART_CANCELLED     : event not yet fired and waiting for the event has been aborted or
     *                              blocking is not allowed anymore. No valid <I>event</I> is returned.
     *      - SMART_DISCONNECTED  : got disconnected while waiting and therefore <I>event</I> not valid
     *                              and also <I>id</I> not valid any longer due to automatic deactivation.
     *      - SMART_NOTACTIVATED  : got deactivated while waiting and therefore <I>event</I> not valid and
     *                              also <I>id</I> not valid any longer.
     *      - SMART_WRONGID       : there is no activation available with this <I>id</I> and therefore
     *                              <I>event</I> not valid.
     *    </p>
     *
     *  - <b>continuous mode</b>:
     *    <p>
     *    Makes sure that only events fired after entering this member function are considered.
     *    </p>
     *    <p>
     *    <b>Returns status code</b>:
     *    </p>
     *    <p>
     *      - SMART_OK            : event fired while waiting for the event and event is consumed
     *                              and returned
     *      - SMART_CANCELLED     : waiting for the next event has been aborted or blocking is not
     *                              allowed anymore. No valid <I>event</I> is returned.
     *      - SMART_DISCONNECTED  : got disconnected while waiting and therefore <I>event</I> not valid
     *                              and also <I>id</I> not valid any longer due to automatic deactivation.
     *      - SMART_NOTACTIVATED  : got deactivated while waiting and therefore <I>event</I> not valid and
     *                              also <I>id</I> not valid any longer.
     *      - SMART_WRONGID       : there is no activation available with this <I>id</I> and therefore
     *                              <I>event</I> not valid.
     *    </p>
     */
    virtual Smart::StatusCode getNextEvent(const Smart::EventIdPtr id, EventType& event, const Smart::Duration &timeout=Smart::Duration::max()) override
    {
    	std::unique_lock<std::recursive_mutex> client_lock(client_mutex);

    	if(disconnected_guard.trigger_value() == true)
    		return Smart::StatusCode::SMART_DISCONNECTED;

    	// downcast the abstract EventId pointer to the DDS-specific pointer type
    	auto dds_id = std::dynamic_pointer_cast<CorrelationId>(id);
    	if(!dds_id)
    		return Smart::StatusCode::SMART_WRONGID;
    	auto foundEventIt = event_cache.find(*dds_id);
    	if(foundEventIt == event_cache.end())
    		return Smart::StatusCode::SMART_WRONGID;

    	try {
    		// this copies the internal shared pointer (which prevents its destruction as long as we have a copy here)
    		auto event_result_ptr = foundEventIt->second;

    		if(event_result_ptr->isSingleMode()) {
    			// here we need to check if the single event has already been fired
    			// so we don't block indefinitely as the event will never fire again
    			if(event_result_ptr->getEventGuard().trigger_value() == true) {
    				return Smart::StatusCode::SMART_PASSIVE;
    			}
    		}

			dds::core::cond::WaitSet wait_set;
			wait_set += disconnected_guard;
			wait_set += nonblocking_guard;

			// the next two guards are specific to the given EventId
			wait_set += event_result_ptr->getDeactivationGuard();

			// each call of getNextEvent() creates an own individual guard that is registered with the current event
			dds::core::cond::GuardCondition next_event_guard;
			wait_set += next_event_guard;
			event_result_ptr->addNextEventGuard(next_event_guard);

			// release the lock to ensure that the patterns remains responsive while this method waits
			client_lock.unlock();

			// wait for one of the provided conditions to become true
			auto active_conditions = wait_set.wait(timeout);

			client_lock.lock();

			if(active_conditions.size() == 0) {
				// if no specified conditions are active, then the only thing that could have happened is a timeout
				return Smart::StatusCode::SMART_TIMEOUT;
			}

			// check which condition caused the unblocking and return the related result
			for(auto condition: active_conditions) {
				if(condition == disconnected_guard) {
					// client has disconnected
					return Smart::StatusCode::SMART_DISCONNECTED;
				} else if(condition == nonblocking_guard) {
					// blocking has been set to false
					return Smart::StatusCode::SMART_CANCELLED;
				} else if(condition == event_result_ptr->getDeactivationGuard()) {
					// event has been deactivated
					return Smart::StatusCode::SMART_NOTACTIVATED;
				} else if(condition == next_event_guard) {
					if(event_result_ptr->hasNewEvent()) {
						// consume the event
						event = event_result_ptr->consumeNextEvent(next_event_guard);
						return Smart::StatusCode::SMART_OK;
					}
				}
			}
		} catch(dds::core::Error &error) {
			std::cerr << error.what() << std::endl;
			return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
		}

    	// something went wrong (this should not happen in normal circumstances)
    	return Smart::StatusCode::SMART_ERROR;
    }
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_EVENTCLIENTPATTERN_H_ */
