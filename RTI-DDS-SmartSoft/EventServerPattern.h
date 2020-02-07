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

#ifndef RTIDDSSMARTSOFT_EVENTSERVERPATTERN_H_
#define RTIDDSSMARTSOFT_EVENTSERVERPATTERN_H_

#include <list>
#include <algorithm>

#include "RTI-DDS-SmartSoft/Component.h"

#include "RTI-DDS-SmartSoft/EventPatternQoS.h"
#include "RTI-DDS-SmartSoft/EventActivationDecorator.h"

#include "RTI-DDS-SmartSoft/DDSReaderConnector.h"
#include "RTI-DDS-SmartSoft/DDSWriterConnector.h"

#include <smartIEventServerPattern_T.h>

namespace SmartDDS {

template<class ActivationType, class EventType, class UpdateType = EventType>
class EventServerPattern
:	public Smart::IEventServerPattern<ActivationType,EventType,UpdateType>
,	public DynamicDataReaderListener
{
private:
	Component* component;

	std::mutex server_mutex;

	std::list<EventActivation<ActivationType>> event_activations;

	EventActivationDecorator activation_decorator;

	DDSReaderConnector dds_reader_connector;
	DDSWriterConnector dds_writer_connector;

	DynamicDataTopic dds_activation_topic;
	DynamicDataReader dds_activation_reader;

	DynamicDataTopic dds_event_topic;
	DynamicDataWriter dds_event_writer;

	virtual void on_data_available(DynamicDataReader& reader) override
	{
		if(this->is_shutting_down())
			return;

		std::unique_lock<std::mutex> lock(server_mutex);

		// we consume the samples, so the internal buffer gets freed
		auto samples = reader.take();
		for(const auto& sample: samples)
		{
			if(sample.info().valid())
			{
				// create an internal event-activation entry
				EventActivation<ActivationType> event_activation(sample.info());

				event_activation.setActivationMode( activation_decorator.extractActivationMode(sample.data()) );

				if(event_activation.isDeactivation()) {
					// we must use the related ID here which is used in the deactivation case
					auto related_id = CorrelationId::createRelatedId(sample.info());
					auto entry_it = std::find(event_activations.begin(), event_activations.end(), related_id);
					if(entry_it != event_activations.end()) {
						event_activations.erase(entry_it);
					}
				} else if(event_activation.isActivation()) {
					// extract event activation parameters object
					ActivationType event_parameters;
					convert(activation_decorator.extractOriginalObject(sample.data()), event_parameters);
					event_activation.setEventParameters( event_parameters );
					// store the event activation in the internal list
					event_activations.push_back(event_activation);

					// notify event activation
					IEventServerBase::onActivation(event_parameters);
				}
			}
		}
	}

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
		std::unique_lock<std::mutex> lock(server_mutex);
		dds_reader_connector.reset(dds_activation_reader);
		component->DDS().resetTopic(dds_activation_topic);
		dds_writer_connector.reset(dds_event_writer);
		component->DDS().resetTopic(dds_event_topic);
	}
public:
	// creating typed aliases, see: https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-types
	using IEventServerBase = Smart::IEventServerPattern<ActivationType,EventType,UpdateType>;
	using typename IEventServerBase::IEventTestHandlerPtr;

	EventServerPattern(Component* component, const std::string& serviceName, IEventTestHandlerPtr testHandler)
	:	IEventServerBase(component, serviceName, testHandler)
	,	component(component)
	,	activation_decorator(dds_type<ActivationType>())
	,	dds_reader_connector(component, EventPatternQoS::getActivationTopicQoS())
	,	dds_writer_connector(component, EventPatternQoS::getEventTopicQoS())
	,	dds_activation_topic(nullptr)
	,	dds_activation_reader(nullptr)
	,	dds_event_topic(nullptr)
	,	dds_event_writer(nullptr)
	{
		auto activationTopicName = component->getName()+"::"+serviceName+"::EventActivationTopic";
		auto eventTopicName = component->getName()+"::"+serviceName+"::EventTopic";

		// the dynamic DDS types are determined using external template methods
		auto dds_activation_type = activation_decorator.getDecoratedDDSType();
		auto dds_event_type = dds_type<EventType>();

		// create the two topics
		dds_activation_topic = component->DDS().findOrCreateTopic(activationTopicName, dds_activation_type);
		dds_event_topic = component->DDS().findOrCreateTopic(eventTopicName, dds_event_type);

		dds_activation_reader = dds_reader_connector.create_new_reader(dds_activation_topic, this);
		dds_event_writer = dds_writer_connector.create_new_writer(dds_event_topic);
	}

	virtual ~EventServerPattern()
	{
		this->serverInitiatedDisconnect();
	}

    /** Initiate testing the event conditions for the activations.
     *
     *  @param state contains the current information checked in testEvent()
     *         against the individual activation parameters.
     *
     *  @return status code
     *   - SMART_OK                  : everything is ok
     *   - SMART_CANCELLED           : server is in the process of shutting down
     *   - SMART_ERROR_COMMUNICATION : communication problems
     *   - SMART_ERROR               : something went wrong
     *
     */
    virtual Smart::StatusCode put(const UpdateType& state) override
    {
		if(this->is_shutting_down())
			return Smart::StatusCode::SMART_CANCELLED;

    	std::unique_lock<std::mutex> lock(server_mutex);

    	// we access the elements by reference (so we can modify them in the loop)
    	for(auto & event_activation: event_activations) {
    		if(event_activation.isContinuous() || !event_activation.hasFiredOnce()) {
    			EventType event;
    			if(IEventServerBase::testEvent(event_activation.getEventParametersRef(), event, state)) {
    				event_activation.fireEvent();
    				try {
    					rti::pub::WriteParams params;
						// set the related event-activation ID as related sample ID
						params.related_sample_identity(event_activation.getEventId());
    					// now write the actual event to the associated client
    					dds_event_writer->write(serialize(event), params);
    				} catch (std::exception &ex) {
						std::cerr << ex.what() << std::endl;
						return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
					}
    			}
    		}
    	}

    	return Smart::StatusCode::SMART_OK;
    }
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_EVENTSERVERPATTERN_H_ */
