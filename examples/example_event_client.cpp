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

#include "RTI-DDS-SmartSoft/EventClientPattern.h"

#include <iostream>
#include <thread>
#include <atomic>
#include <functional>
#include <future>

class MyEventHandler: public SmartDDS::EventHandler<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d> {
public:
	MyEventHandler(SmartDDS::EventClientPattern<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d> *eventClient)
	:	SmartDDS::EventHandler<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d>(eventClient)
	{  }
	virtual ~MyEventHandler() = default;

	virtual void handleEvent(const Smart::EventIdPtr &id, const CommExampleObjects::CommPose6d& event) override
	{
		std::cout << "handle event: " << event << std::endl;
	}
};

class ClientApplication {
private:
	SmartDDS::Component component;

	std::string serverName;
	std::string serviceName;

	SmartDDS::EventClientPattern<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d> eventClient;
	MyEventHandler handler;

	std::atomic<bool> cancelled;

	void execute_application() {
		while(!cancelled) {
			CommExampleObjects::CommPosition params;
			params.x = 10;
			Smart::EventIdPtr id;

			auto status = eventClient.activate(Smart::continuous, params, id);
			if(status == Smart::StatusCode::SMART_DISCONNECTED) {
				status = eventClient.connect(serverName, serviceName);
				std::cout << "connection "  << status << std::endl;
				continue;
			}

			for(auto i=0; i<10; ++i) {
				CommExampleObjects::CommPose6d event;
				std::cout << "wait for event: " << id << std::endl;
				status = eventClient.getEvent(id, event);
				if(status == Smart::StatusCode::SMART_OK) {
					std::cout << "received event: " << event << std::endl;
				} else {
					std::cout << status << std::endl;
				}
			}

			std::cout << "deactivate event: " << eventClient.deactivate(id) << std::endl;
		}
	}

public:
	ClientApplication(const std::string &componentName, const std::string &serverName, const std::string &serviceName)
	:	component(componentName)
	,	serverName(serverName)
	,	serviceName(serviceName)
	,	eventClient(&component)
	,	handler(&eventClient)
	,	cancelled(false)
	{  }

	void run() {
		std::thread clientThread = std::thread(std::bind(&ClientApplication::execute_application, this));
		component.run();
		cancelled = true;
		if(clientThread.joinable()) {
			clientThread.join();
		}
	}
};


int main(int argc, char* argv[]) {
	std::string componentName = "EventClient";
	if(argc > 1) {
		componentName = argv[1];
	}
	std::string serverName = "EventServer";
	std::string serviceName = "TextService";

	ClientApplication application(componentName, serverName, serviceName);
	application.run();
	return 0;
}
