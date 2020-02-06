/*
 * example_query_client.cpp
 *
 *  Created on: Jun 11, 2019
 *      Author: alexej
 */

#include "CommPose6d.h"
#include "CommPose6dDDS.h"

#include "RtiDdsSmartSoft/QueryClientPattern.h"

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
