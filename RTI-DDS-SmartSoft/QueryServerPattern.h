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

#ifndef RTIDDSSMARTSOFT_QUERYSERVERPATTERN_H_
#define RTIDDSSMARTSOFT_QUERYSERVERPATTERN_H_

#include <set>
#include <list>
#include <mutex>

#include "RTI-DDS-SmartSoft/Component.h"
#include "RTI-DDS-SmartSoft/CorrelationId.h"
#include "RTI-DDS-SmartSoft/QueryPatternQoS.h"
#include "RTI-DDS-SmartSoft/QueryServerHandler.h"

#include "RTI-DDS-SmartSoft/DDSReaderConnector.h"
#include "RTI-DDS-SmartSoft/DDSWriterConnector.h"

#include <smartIQueryServerPattern_T.h>

namespace SmartDDS {

template<class RequestType, class AnswerType>
class QueryServerPattern
:	public Smart::IQueryServerPattern<RequestType,AnswerType>
,	public DynamicDataReaderListener
{
private:
	Component* component;

	std::mutex server_mutex;
	std::set<Smart::QueryIdPtr> request_cache;
	std::list<dds::core::InstanceHandle> connected_clients;

	DDSReaderConnector dds_reader_connector;
	DDSWriterConnector dds_writer_connector;

	DynamicDataTopic dds_request_topic;
	DynamicDataReader dds_request_reader;

	DynamicDataTopic dds_reply_topic;
	DynamicDataWriter dds_reply_writer;

    virtual void on_liveliness_changed(
    	DynamicDataReader &reader,
        const LivelinessChangedStatus &status) override
    {
    	if(this->is_shutting_down())
    				return;

    	std::unique_lock<std::mutex> lock(server_mutex);

    	auto client_handle = status.last_publication_handle();
    	if(status.alive_count_change() > 0) {
    		auto clientIt = std::find(connected_clients.cbegin(), connected_clients.cend(), client_handle);
    		if(clientIt == connected_clients.end()) {
    			connected_clients.push_back(client_handle);
    		}
    	} else if(status.alive_count_change() < 0) {
    		connected_clients.remove(client_handle);
    	}
    }

	virtual void on_data_available(DynamicDataReader& reader) override
	{
		if(this->is_shutting_down())
			return;

		// we consume the samples, so the internal buffer gets freed
		auto requests = reader.take();
		for(const auto& request: requests)
		{
			if(request.info().valid())
			{
				RequestType request_object;
				// convert the request data into the user-level object
				convert(request.data(), request_object);
				// get the original sample identity (aka QueryId) from the request info object
				auto query_id = std::make_shared<CorrelationId>(request.info());

				std::unique_lock<std::mutex> lock(server_mutex);
				// we additionally store the request id inside an internal request cache for
				// later validation within the answer method
				request_cache.insert(query_id);
				// release the lock before calling notify to ensure responsiveness of the pattern
				lock.unlock();

				// propagate handle query request to the base class (which internally uses the registered handler)
				IQueryServerBase::handleQuery(query_id, request_object);

				// check if server has been commanded to shutdown in the meantime, and if so, stop the loop
				if(this->is_shutting_down())
					break;
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
		request_cache.clear();
		connected_clients.clear();
		dds_reader_connector.reset(dds_request_reader);
		component->DDS().resetTopic(dds_request_topic);
		dds_writer_connector.reset(dds_reply_writer);
		component->DDS().resetTopic(dds_reply_topic);
	}
public:
	using IQueryServerBase = Smart::IQueryServerPattern<RequestType,AnswerType>;
	using typename IQueryServerBase::IQueryServerHandlerPtr;

	QueryServerPattern(Component* component, const std::string& serviceName, IQueryServerHandlerPtr query_handler = nullptr)
	:	IQueryServerBase(component, serviceName, query_handler)
	,	component(component)
	,	dds_reader_connector(component, QueryPatternQoS::getRequestTopicQoS())
	,	dds_writer_connector(component, QueryPatternQoS::getReplyTopicQoS())
	,	dds_request_topic(nullptr)
	,	dds_request_reader(nullptr)
	,	dds_reply_topic(nullptr)
	,	dds_reply_writer(nullptr)
	{
		auto requestTopicName = component->getName()+"::"+serviceName+"::RequestTopic";
		auto replyTopicName = component->getName()+"::"+serviceName+"::ReplyTopic";

		// the dynamic DDS types are determined using external template methods
		auto dds_request_type = dds_type<RequestType>();
		auto dds_answer_type = dds_type<AnswerType>();

		// create the two topics
		dds_request_topic = component->DDS().findOrCreateTopic(requestTopicName, dds_request_type);
		dds_reply_topic = component->DDS().findOrCreateTopic(replyTopicName, dds_answer_type);

		dds_request_reader = dds_reader_connector.create_new_reader(dds_request_topic, this);
		dds_reply_writer = dds_writer_connector.create_new_writer(dds_reply_topic);
	}
	virtual ~QueryServerPattern()
	{
		this->serverInitiatedDisconnect();
	}

    /** Provide answer to be sent back to the requestor.
     *
     *  Member function is thread safe and thread reentrant.
     *
     *  @param id identifies the request to which the answer belongs
     *  @param answer is the reply itself.
     *
     *  @return status code:
     *    - SMART_OK                  : everything is ok and answer sent to requesting client
     *    - SMART_WRONGID             : no pending query with that <I>id</I> known
     *    - SMART_DISCONNECTED        : answer not needed anymore since client
     *                                  got disconnected meanwhile
     *    - SMART_ERROR_COMMUNICATION : communication problems
     *    - SMART_ERROR               : something went wrong
     */
    virtual Smart::StatusCode answer(const Smart::QueryIdPtr id, const AnswerType& answer) override
    {
		if(this->is_shutting_down())
			return Smart::StatusCode::SMART_DISCONNECTED;

		// we uniquely lock the mutex for the entire method as the whole execution
		// here should be comparably fast
		std::unique_lock<std::mutex> lock(server_mutex);

		// 1. check if the provided QueryId is valid
		auto foundRequestId = request_cache.find(id);
		if (foundRequestId == request_cache.end()) {
			return Smart::StatusCode::SMART_WRONGID;
		}
		// here we downcast our shared pointer to the dds pointer type
		auto dds_id = std::dynamic_pointer_cast<CorrelationId>(id);
		if(!dds_id) {
			// this case should never happen, but just in case...
			return Smart::StatusCode::SMART_WRONGID;
		}

		// 2. check if related client still is connected
		auto clientIt = std::find(connected_clients.cbegin(),
				connected_clients.cend(), dds_id->getWriterHandle());
		if (clientIt == connected_clients.end()) {
			return Smart::StatusCode::SMART_DISCONNECTED;
		}

		try {
			rti::pub::WriteParams params;
			// 3. set the related query ID as related sample ID
			params.related_sample_identity(*dds_id);

			// 4. send the actual answer along with the related query ID
			dds_reply_writer->write(serialize(answer), params);

			// 5. clean-up the cache
			request_cache.erase(foundRequestId);
		} catch (std::exception &ex) {
			std::cerr << ex.what() << std::endl;
			return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
		}
		// all error cases have been checked and passed, so answer was successful
		return Smart::StatusCode::SMART_OK;
    }
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_QUERYSERVERPATTERN_H_ */
