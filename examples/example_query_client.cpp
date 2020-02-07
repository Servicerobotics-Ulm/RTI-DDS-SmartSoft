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

#include "RTI-DDS-SmartSoft/QueryClientPattern.h"

#include <iostream>
#include <thread>
#include <atomic>
#include <functional>
#include <future>

class ClientApplication {
private:
	SmartDDS::Component component;

	std::string serverName;
	std::string serviceName;

	SmartDDS::QueryClientPattern<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d> queryClient;

	std::atomic<bool> cancelled;

	void execute_application() {
		int counter = 0;
		while(!cancelled) {
			CommExampleObjects::CommPosition request;
			CommExampleObjects::CommPose6d answer;
			Smart::QueryIdPtr id;
			request.x = counter++;
			auto status = queryClient.query(request, answer);
			if(status == Smart::StatusCode::SMART_OK) {
//				std::cout << "send request: "<< request.x << " ..." << std::endl;
//				std::this_thread::sleep_for(std::chrono::seconds(1));
//				status = queryClient.queryReceiveWait(id, answer);
				std::cout << "received answer: " << answer.position.x << " with " << status << std::endl;
			} else {
				std::cout << status << std::endl;
				std::cout << "trying to connect to " << serverName << "::" << serviceName << "..." << std::endl;
				status = queryClient.connect(serverName, serviceName);
				std::cout << "connection status: " << status << std::endl;
//				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}

	}

public:
	ClientApplication(const std::string &componentName, const std::string &serverName, const std::string &serviceName)
	:	component(componentName)
	,	serverName(serverName)
	,	serviceName(serviceName)
	,	queryClient(&component)
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
	std::string componentName = "QueryClient";
	if(argc > 1) {
		componentName = argv[1];
	}
	std::string serverName = "QueryServer";
	std::string serviceName = "TextService";

	ClientApplication application(componentName, serverName, serviceName);
	application.run();
	return 0;
}
