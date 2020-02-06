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

#ifndef RTIDDSSMARTSOFT_QUERYSERVERHANDLER_H_
#define RTIDDSSMARTSOFT_QUERYSERVERHANDLER_H_

#include <smartIQueryServerPattern_T.h>

namespace SmartDDS {

// forward declaration
template<class RequestType, class AnswerType>
class IQueryServerPattern;

/** Handler Class for QueryServer for incoming requests.
 *
 *  Used by the QueryServer to handle incoming queries.
 *  The user should implement the handleQuery() method by
 *  subclassing and providing a pointer to an IQueryServerPattern
 *  to this handler.
 */
template<class RequestType, class AnswerType>
class QueryServerHandler : public Smart::IQueryServerHandler<RequestType,AnswerType>
{
public:
	/** Default destructor
	 */
	virtual ~QueryServerHandler() = default;

	// this alias can be used to simplify implementation of the handleQuery method in derived classes
	using IQueryServer = Smart::IQueryServerPattern<RequestType,AnswerType>;

	/** Handler method for an incoming query request.
	 *
	 *  This method is called by the query-server every time
	 *  a new query request is received. It must be provided by the
	 *  component developer to handle incoming requests. Since the
	 *  method is executed by the communication thread, it must be
	 *  very fast and non-blocking. Within this handler, use the
	 *  provided <b>server</b> pointer to provide an answer like
	 *  this: <b>"server->answer(...)"</b>.
	 *
	 *  Usually the request and the id will be inserted into a queue
	 *  and another working thread processes the request and provides
	 *  the result. The ThreadedQueryHandler decorator provides such
	 *  a processing pattern.
	 *
	 *  @param server   a reference to the related query server pattern for calling the answer(...) method from within the handler
	 *  @param id       id of new query
	 *  @param request  the request itself
	 */
	virtual void handleQuery(IQueryServer &server, const Smart::QueryIdPtr &id, const RequestType& request) = 0;
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_QUERYSERVERHANDLER_H_ */
