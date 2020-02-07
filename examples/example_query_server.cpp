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
#include "CommPose6d.h"
#include "CommPose6dDDS.h"

#include "RTI-DDS-SmartSoft/QueryServerPattern.h"
#include "RTI-DDS-SmartSoft/ProcessingPatterns.h"

#include <iostream>

class MyStringHandler
:	public SmartDDS::QueryServerHandler<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d>
{
private:
	  /** Handler method for an incoming command.
	   *
	   *  This method is called by the ISendServerPattern every time
	   *  a new data is received. It must be provided by the component
	   *  developer to handle incoming data. Since the method is
	   *  executed by the communication thread, it must be very fast
	   *  and non-blocking.
	   *
	   *  Usually the data will be inserted into a queue and another
	   *  working thread processes the command. The IActiveQueueInputHandlerDecorator
	   *  provides such a processing pattern.
	   *
	   *  @param data communicated DataType object (Communication Object)
	   */
	  virtual void handleQuery(IQueryServer &server, const Smart::QueryIdPtr &id, const CommExampleObjects::CommPosition& request) override
	  {
		  std::cout << "incoming query request " << request.x << std::endl;

		  CommExampleObjects::CommPose6d response;
		  response.position = request;

		  std::this_thread::sleep_for(std::chrono::seconds(1));
		  auto status = server.answer(id, response);
		  std::cout << "replied " << response.position.x << " with status " << status << std::endl;
	  }
public:
	virtual ~MyStringHandler() = default;
};

using ActiveStringHandler = SmartDDS::ThreadQueueQueryHandler<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d>;

class ServerApplication {
private:
	SmartDDS::Component component;
	std::shared_ptr<MyStringHandler> handler;
	std::shared_ptr<ActiveStringHandler> active_handler;
	SmartDDS::QueryServerPattern<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d> queryServer;

public:
	ServerApplication(const std::string &componentName, const std::string &serviceName)
	:	component(componentName)
	,	handler(std::make_shared<MyStringHandler>())
	,	active_handler(std::make_shared<ActiveStringHandler>(&component, handler))
	,	queryServer(&component, serviceName, handler)
	{  }

	void run() {
		std::cout << "ServerApplication started" << std::endl;
		component.run();
	}
};

int main(int argc, char* argv[]) {
	std::string componentName = "QueryServer";
	if(argc > 1) {
		componentName = argv[1];
	}
	std::string serviceName = "TextService";

	ServerApplication application(componentName, serviceName);
	application.run();
	return 0;
}


