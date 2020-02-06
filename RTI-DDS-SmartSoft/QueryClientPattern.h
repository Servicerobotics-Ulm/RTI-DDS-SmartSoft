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

#ifndef RTIDDSSMARTSOFT_QUERYCLIENTPATTERN_H_
#define RTIDDSSMARTSOFT_QUERYCLIENTPATTERN_H_

#include <map>
#include <mutex>
#include <memory>

#include "RtiDdsSmartSoft/Component.h"
#include "RtiDdsSmartSoft/CorrelationId.h"

#include "RtiDdsSmartSoft/DDSReaderConnector.h"
#include "RtiDdsSmartSoft/DDSWriterConnector.h"

#include "RtiDdsSmartSoft/QueryPatternQoS.h"
#include "RtiDdsSmartSoft/QueryClientAnswerTrigger.h"

#include <smartIQueryClientPattern_T.h>

namespace SmartDDS {

template<class RequestType, class AnswerType>
class QueryClientPattern
:	public Smart::IQueryClientPattern<RequestType, AnswerType>
,	public DynamicDataReaderListener
{
private:

	Component *component;

	std::recursive_mutex connection_mutex;

	std::recursive_mutex answer_mutex;
	std::map<CorrelationId, std::shared_ptr<QueryClientAnswerTrigger<AnswerType>>> answer_cache;

	// this helpers allow checking if a remote end-point actually responds during a connection phase (see connect(...) method)
	DDSWriterConnector dds_writer_connector;
	DDSReaderConnector dds_reader_connector;

	DynamicDataTopic dds_request_topic;
	DynamicDataWriter dds_request_writer;

	DynamicDataTopic dds_reply_topic;
	DynamicDataFilteredTopic dds_filtered_reply_topic;
	DynamicDataReader dds_filtered_reply_reader;

	dds::core::cond::GuardCondition disconnected_guard;
	dds::core::cond::GuardCondition nonblocking_guard;

    void on_liveliness_changed(DynamicDataReader&, const LivelinessChangedStatus &status)
    {
    	if(status.alive_count() == 0) {
    		disconnected_guard.trigger_value(true);
    	} else if(status.alive_count() > 0) {
    		disconnected_guard.trigger_value(false);
    	}
    }
    void on_data_available(DynamicDataReader &reader)
    {
    	// do not grab the mutex if client is in the process of shutting down or disconnecting
		if(this->is_shutting_down() || disconnected_guard.trigger_value() == true)
			return;

		// consume all incoming answers
		auto answers = reader.take();
		for(auto answer: answers) {
			if(answer.info().valid()) {
				std::unique_lock<std::recursive_mutex> answer_lock(answer_mutex);
				auto answer_id = CorrelationId::createRelatedId(answer.info());
				auto query_it = answer_cache.find(answer_id);
				if(query_it != answer_cache.end()) {
					// this call overrides the internal answer object copy for the given ID
					query_it->second->triggerNewAnswerData(answer.data());
				}
			}
		}
    }

public:
    /** Constructor (not wired with service provider and not exposed as port).
     *  add()/remove() and connect()/disconnect() can always be used to change
     *  the status of the instance. Instance is not connected to a service provider
     *  and is not exposed as port wireable from outside the component.
     *
     *  (Currently exception not thrown)
     *
     * @param component  management class of the component
     */
	QueryClientPattern(Component* component)
	:	Smart::IQueryClientPattern<RequestType, AnswerType>(component)
	,	component(component)
	,	dds_writer_connector(component, QueryPatternQoS::getRequestTopicQoS())
	,	dds_reader_connector(component, QueryPatternQoS::getReplyTopicQoS())
	,	dds_request_topic(nullptr)
	,	dds_request_writer(nullptr)
	,	dds_reply_topic(nullptr)
	,	dds_filtered_reply_topic(nullptr)
	,	dds_filtered_reply_reader(nullptr)
	{
		// by default, the client initializes in the disconnected state
		disconnected_guard.trigger_value(true);
	}

    /** Constructor (wired with specified service provider).
     *  Connects to the denoted service and blocks until the connection
     *  has been established. Blocks infinitely if denoted service becomes
     *  not available since constructor performs retries. Blocking is useful to
     *  simplify startup of components which have mutual dependencies.
     *  add()/remove() and connect()/disconnect() can always be used to change
     *  the status of the instance.
     *
     *  Throws exception if denoted service is incompatible (wrong communication
     *  pattern or wrong communication objects).
     *    - SMART_INCOMPATIBLESERVICE : the denoted service is incompatible (wrong communication
     *                                  pattern or wrong communication objects) and can therefore
     *                                  not be connected. Instance is not created.
     *    - SMART_ERROR               : something went wrong, instance not created
     *
     * @param component  management class of the component
     * @param server     name of the server
     * @param service    name of the service
     */
	QueryClientPattern(Component* component, const std::string& server, const std::string& service)
	:	Smart::IQueryClientPattern<RequestType, AnswerType>(component)
	,	component(component)
	,	dds_writer_connector(component, QueryPatternQoS::getRequestTopicQoS())
	,	dds_reader_connector(component, QueryPatternQoS::getReplyTopicQoS())
	,	dds_request_topic(nullptr)
	,	dds_request_writer(nullptr)
	,	dds_reply_topic(nullptr)
	,	dds_filtered_reply_topic(nullptr)
	,	dds_filtered_reply_reader(nullptr)
	{
		// by default, the client initializes in the disconnected state
		disconnected_guard.trigger_value(true);

		this->connect(server, service);
	}

    /** Destructor.
     *  The destructor calls remove() and disconnect() and therefore properly cleans up
     *  every pending query and removes the instance from the set of wireable ports.
     */
	virtual ~QueryClientPattern()
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

		auto requestTopicName = server+"::"+service+"::RequestTopic";
		auto replyTopicName = server+"::"+service+"::ReplyTopic";

		// the dynamic DDS types are determined using external template methods
		auto dds_request_type = dds_type<RequestType>();
		auto dds_answer_type = dds_type<AnswerType>();

		try {
			// if the related query server is in the same component, then the topic is already defined and we can simply reuse it
			dds_request_topic = component->DDS().findOrCreateTopic(requestTopicName, dds_request_type);
			dds_reply_topic = component->DDS().findOrCreateTopic(replyTopicName, dds_answer_type);

			auto timeout = std::chrono::seconds(1);
			auto connection_status = dds_writer_connector.reconnect(dds_request_writer, dds_request_topic, timeout);

			if(connection_status != Smart::StatusCode::SMART_OK) {
				this->disconnect();
				return connection_status;
			}

			// initiate the connection ID from the request writer
			ConnectionId requester_id(dds_request_writer);

			// create a content filtered topic
			dds_filtered_reply_topic = component->DDS().findOrCreateClientFilteredTopic(dds_reply_topic, requester_id);

			// reconnect the reader to the filtered topic
			connection_status = dds_reader_connector.reconnect(dds_filtered_reply_reader, dds_filtered_reply_topic, timeout, this);
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

		std::unique_lock<std::recursive_mutex> answer_lock(answer_mutex);
		for(const auto& query: answer_cache) {
			query.second->triggerDiscard();
		}
		answer_cache.clear();
		answer_lock.unlock();

		std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);

		// release the resource for the request-channel
		dds_writer_connector.reset(dds_request_writer);
		component->DDS().resetTopic(dds_request_topic);

		dds_reader_connector.reset(dds_filtered_reply_reader);
		component->DDS().resetFilteredTopic(dds_filtered_reply_topic);
		component->DDS().resetTopic(dds_reply_topic);

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
    	std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);
    	this->is_blocking = blocking;
    	nonblocking_guard.trigger_value(blocking);
    	return Smart::StatusCode::SMART_OK;
    }

    /** Blocking Query.
     *
     *  Perform a blocking query and return only when the query answer
     *  is available. Member function is thread safe and thread reentrant.
     *
     *  @param request send this request to the server (Communication Object)
     *  @param answer  returned answer from the server (Communication Object)
     *
     *  @return status code:
     *    - SMART_OK                  : everything is ok and <I>answer</I> contains answer
     *    - SMART_CANCELLED           : blocking is not allowed or is not allowed anymore and therefore
     *                                  pending query is aborted, answer is lost and <I>answer</I>
     *                                  contains no valid answer.
     *    - SMART_DISCONNECTED        : the client is either disconnected and no query
     *                                  can be made or it got disconnected and a pending
     *                                  query is aborted without answer. In both cases,
     *                                  <I>answer</I> is not valid.
     *    - SMART_ERROR_COMMUNICATION : communication problems, <I>answer</I> is not valid.
     *    - SMART_ERROR               : something went wrong, <I>answer</I> is not valid.
     */
    using Smart::IQueryClientPattern<RequestType,AnswerType>::query;

    /** Asynchronous Query.
     *
     *  Perform a query and receive the answer later, returns immediately.
     *  Member function is thread safe and reentrant.
     *
     *  @param request send this request to the server (Communication Object)
     *  @param id      is set to the identifier which is later used to receive
     *                 the reply to this request
     *
     *  @return status code:
     *    - SMART_OK                  : everything is ok and <I>id</I> contains query identifier
     *                                  used to either fetch or discard the answer.
     *    - SMART_DISCONNECTED        : request is rejected since client is not connected to a server
     *                                  and therefore <I>id</I> is not a valid identifier.
     *    - SMART_ERROR_COMMUNICATION : communication problems, <I>id</I> is not valid.
     *    - SMART_ERROR               : something went wrong, <I>id</I> is not valid.
     */
	virtual Smart::StatusCode queryRequest(const RequestType& request, Smart::QueryIdPtr& id) override
	{
		if (disconnected_guard.trigger_value() == true)
			return Smart::StatusCode::SMART_DISCONNECTED;

		try {
			std::unique_lock<std::recursive_mutex> connection_lock(connection_mutex);

			// use write-parameters to determine the generated sample ID
			rti::pub::WriteParams params;
			params.replace_automatic_values(true);

			// sends the query request (using the extended write method)
			dds_request_writer->write(serialize(request), params);
			// get the generated sample ID
			auto sample_id = params.identity();
			id = std::make_shared<CorrelationId>(sample_id);

			std::unique_lock<std::recursive_mutex> answer_lock(answer_mutex);
			// create an answer-trigger that will be used to store the received answer
			// and trigger all blocking queryReceiveWait calls to release for the given ID
			answer_cache[sample_id] = std::make_shared<QueryClientAnswerTrigger<AnswerType>>();
			return Smart::StatusCode::SMART_OK;
		} catch (std::exception &ex) {
			std::cerr << ex.what() << std::endl;
			return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
		}

		return Smart::StatusCode::SMART_ERROR;
	}

    /** Check if answer is available.
     *
     *  Non-blocking call to fetch the answer belonging to the given identifier.
     *  Returns immediately. Member function is thread safe and reentrant.
     *
     *  @warning
     *    It is not allowed to call queryReceive(), queryReceiveWait() or queryDiscard() concurrently
     *    with the <I>same</I> query id (which is not a restriction since it makes no sense !)
     *
     *  @param id      provides the identifier of the query
     *  @param answer  is set to the answer returned from the server if it was available
     *
     *  @return status code:
     *    - SMART_OK           : everything is ok and <I>answer</I> contains the answer
     *    - SMART_WRONGID      : no pending query with this identifier available, therefore no valid
     *                           <I>answer</I> returned.
     *    - SMART_NODATA       : answer not yet available, therefore try again later. The identifier <I>id</I>
     *                           keeps valid, but <I>answer</I> contains no valid answer.
     *    - SMART_DISCONNECTED : the answer belonging to the <I>id</I> can not be received
     *                           anymore since the client got disconnected. <I>id</I> is
     *                           not valid any longer and <I>answer</I> contains no valid answer.
     *    - SMART_ERROR        : something went wrong, <I>answer</I> contains no answer and <I>id</I> is
     *                           not valid any longer.
     *
     */
    virtual Smart::StatusCode queryReceive(const Smart::QueryIdPtr id, AnswerType& answer) override
    {
    	if(disconnected_guard.trigger_value() == true)
    		return Smart::StatusCode::SMART_DISCONNECTED;

    	std::unique_lock<std::recursive_mutex> answer_lock(answer_mutex);

    	auto dds_id = std::dynamic_pointer_cast<CorrelationId>(id);
    	if(!dds_id) {
    		return Smart::StatusCode::SMART_WRONGID;
    	}
    	auto foundQueryIt = answer_cache.find(*dds_id);
    	if(foundQueryIt == answer_cache.end()) {
    		// the requested ID is not within the internal pending queries cache (so it is invalid)
    		return Smart::StatusCode::SMART_WRONGID;
    	}

    	if(foundQueryIt->second->hasAnswer()) {
    		// copy the answer object to the answer out-value
    		answer = foundQueryIt->second->getAnswerObject();
    		// as we have consumed the answer, we can free the cache entry
    		answer_cache.erase(foundQueryIt);
    		return Smart::StatusCode::SMART_OK;
    	} else {
    		return Smart::StatusCode::SMART_NODATA;
    	}

    	// something went wrong when trying to read the received query-answer (this should not happen in normal circumstances)
    	return Smart::StatusCode::SMART_ERROR;
    }

    /** Wait for reply.
     *
     *  Blocking call to fetch the answer belonging to the given identifier. Waits until
     *  the answer is received.
     *
     *  @warning
     *    It is not allowed to call queryReceive(), queryReceiveWait() or queryDiscard() concurrently
     *    with the <I>same</I> query id (which is not a restriction since it makes no sense !)
     *
     *  @param id       provides the identifier of the query
     *  @param answer   is set to the answer returned from the server if it was available
     *  @param timeout is the timeout time to block the method maximally (default value zero block infinitelly)
     *
     *  @return status code:
     *    - SMART_OK           : everything is ok and <I>answer</I> contains the answer
     *    - SMART_WRONGID      : no pending query with this identifier available, therefore no
     *                           valid <I>answer</I> returned.
     *    - SMART_CANCELLED    : blocking call is not allowed or is not allowed anymore and therefore
     *                           blocking call is aborted and no valid <I>answer</I> is returned. The
     *                           query identifier <I>id</I> keeps valid and one can either again call
     *                           queryReceive(), queryReceiveWait() or discard the answer by calling
     *                           queryDiscard().
     *    - SMART_DISCONNECTED : blocking call is aborted and the answer belonging to <I>id</I> can not
     *                           be received anymore since client got disconnected. <I>id</I> is not valid
     *                           any longer and <I>answer</I> contains no valid answer.
     *    - SMART_TIMEOUT      : a timeout occurred before an answer has been received
     *    - SMART_ERROR        : something went wrong, <I>answer</I> contains no answer and <I>id</I> is
     *                           not valid any longer.
     *
     */
    virtual Smart::StatusCode queryReceiveWait(const Smart::QueryIdPtr id, AnswerType& answer, const Smart::Duration &timeout = Smart::Duration::max()) override
    {
    	if(disconnected_guard.trigger_value() == true)
    		return Smart::StatusCode::SMART_DISCONNECTED;

    	std::unique_lock<std::recursive_mutex> answer_lock(answer_mutex);

    	auto dds_id = std::dynamic_pointer_cast<CorrelationId>(id);
    	if(!dds_id) {
    		return Smart::StatusCode::SMART_WRONGID;
    	}
    	auto foundQueryIt = answer_cache.find(*dds_id);
    	if(foundQueryIt == answer_cache.end()) {
    		// the requested ID is not within the internal pending queries cache (so it is invalid)
    		return Smart::StatusCode::SMART_WRONGID;
    	}

    	try {
    		// here we copy the shared pointer so its content will not be deleted unintentionally
    		// while we wait for an answer below
    		auto answer_ptr = foundQueryIt->second;

			dds::core::cond::WaitSet wait_set;
			wait_set += disconnected_guard;
			wait_set += nonblocking_guard;

			wait_set += answer_ptr->getDiscardGuard();
			wait_set += answer_ptr->getResultGuard();

			// unlock the connection mutex so the pattern remains responsive while waiting
			answer_lock.unlock();

			// blocking wait until one of the specified conditions becomes true (or a timeout occurs)
			auto active_conditions = wait_set.wait(timeout);

			// acquire the lock back, so we can safely access the answer_ptr
			answer_lock.lock();

			if(active_conditions.size() == 0) {
				// if no specified conditions are active, then the only thing that could have happened is a timeout
				return Smart::StatusCode::SMART_TIMEOUT;
			}

			// check the active conditions
			for(auto condition: active_conditions) {
				// we only need to check the non-blocking guard explicitly here,
				// all the other cases are checked within the getUpdate(...) call below
				if(condition == disconnected_guard) {
					return Smart::StatusCode::SMART_DISCONNECTED;
				} else if(condition == nonblocking_guard) {
					return Smart::StatusCode::SMART_CANCELLED;
				} else if(condition == answer_ptr->getDiscardGuard()) {
					// it can be the case that disconnected has been triggered in parallel
					if(disconnected_guard.trigger_value() == true) {
						return Smart::StatusCode::SMART_DISCONNECTED;
					}
					return Smart::StatusCode::SMART_CANCELLED;
				} else if(condition == answer_ptr->getResultGuard()) {
					answer = answer_ptr->getAnswerObject();
					answer_cache.erase(foundQueryIt);
					return Smart::StatusCode::SMART_OK;
				}
			}
		} catch(dds::core::Error &error) {
			return Smart::StatusCode::SMART_ERROR_COMMUNICATION;
		}

    	// something went wrong when trying to read the received query-answer (this should not happen in normal circumstances)
    	return Smart::StatusCode::SMART_ERROR;
    }

    /** Discard the pending answer with the identifier <I>id</I>
     *
     *  Call this member function if you do not want to get the answer of a request anymore which
     *  was invoked by queryRequest(). This member function invalidates the identifier <I>id</I>.
     *
     *  @warning
     *    This member function does NOT abort blocking calls ! This is done by the blocking() member
     *    function. It has to be called if you have not yet received an answer and the identifier is
     *    still valid, for example due to a CANCELLED return value, and you don't want to get the
     *    answer anymore.
     *
     *  @warning
     *    It is not allowed to call queryReceive(), queryReceiveWait() or queryDiscard() concurrently
     *    with the <I>same</I> query id (which is not a restriction since it makes no sense !)
     *
     *  @param id  provides the identifier of the query
     *
     *  @return status code:
     *    - SMART_OK           : everything is ok and query with the identifier <I>id</I> discarded.
     *    - SMART_WRONGID      : no pending query with this identifier.
     *    - SMART_ERROR        : something went wrong, <I>id</I> not valid any longer.
     *
     */
	virtual Smart::StatusCode queryDiscard(const Smart::QueryIdPtr id) override
	{
		std::unique_lock<std::recursive_mutex> answer_lock(answer_mutex);

		auto dds_id = std::dynamic_pointer_cast<CorrelationId>(id);
		if (!dds_id) {
			return Smart::StatusCode::SMART_WRONGID;
		}
		auto foundQueryIt = answer_cache.find(*dds_id);
		if (foundQueryIt == answer_cache.end()) {
			return Smart::StatusCode::SMART_WRONGID;
		}
		// release all blocking queryReceiveWait() calls for the specified query ID
		foundQueryIt->second->triggerDiscard();
		// remove the map entry
		answer_cache.erase(foundQueryIt);

		return Smart::StatusCode::SMART_OK;
	}
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_QUERYCLIENTPATTERN_H_ */
