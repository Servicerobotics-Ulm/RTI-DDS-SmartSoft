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
#include "CommText.h"
#include "CommTextDDS.h"

#include "RTI-DDS-SmartSoft/SendServerPattern.h"

#include <iostream>

class MyStringHandler
:	public Smart::ISendServerHandler<CommExampleObjects::CommText>
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
	  virtual void handleSend(const CommExampleObjects::CommText& data) override
	  {
		  std::cout << "handleSend(" << data.text << ")" << std::endl;
	  }
public:
	virtual ~MyStringHandler() = default;
};

class ServerApplication {
private:
	SmartDDS::Component component;
	std::shared_ptr<MyStringHandler> handler;
	SmartDDS::SendServerPattern<CommExampleObjects::CommText> stringServer;

public:
	ServerApplication(const std::string &componentName, const std::string &serviceName)
	:	component(componentName)
	,	handler(std::make_shared<MyStringHandler>())
	,	stringServer(&component, serviceName, handler)
	{  }

	void run() {
		std::cout << "ServerApplication started" << std::endl;
		component.run();
	}
};

int main(int argc, char* argv[]) {
	std::string componentName = "SendServer";
	if(argc > 1) {
		componentName = argv[1];
	}
	std::string serviceName = "TextService";

	ServerApplication application(componentName, serviceName);
	application.run();
	return 0;
}


