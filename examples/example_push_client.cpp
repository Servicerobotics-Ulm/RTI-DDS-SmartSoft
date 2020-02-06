/*
 * example_push_client.cpp
 *
 *  Created on: Jun 5, 2019
 *      Author: alexej
 */

#include "CommPosition.h"
#include "CommPositionDDS.h"

#include "RtiDdsSmartSoft/PushClientPattern.h"

#include <iostream>
#include <thread>
#include <functional>

class ClientApplication {
private:
	SmartDDS::Component component;
	SmartDDS::PushClientPattern<CommExampleObjects::CommPosition> stringClient;

	std::string serverName;
	std::string serviceName;

	bool cancelled;

	void execute_application() {
		while(!cancelled) {
			CommExampleObjects::CommPosition position;
			auto status = stringClient.getUpdateWait(position);
			std::cout << "status " << status << std::endl;
			if(status != Smart::StatusCode::SMART_OK) {
				std::cout << "trying to connect to " << serverName << "::" << serviceName << "..." << std::endl;
				status = stringClient.connect(serverName, serviceName);
				std::cout << "connection status: " << status << std::endl;
				status = stringClient.subscribe(2);
				std::cout << "subscription status " << status << std::endl;
			} else {
				std::cout << "received position: " << position.x << std::endl;
			}
		}
	}

public:
	ClientApplication(const std::string &componentName, const std::string &serverName, const std::string &serviceName)
	:	component(componentName)
	,	stringClient(&component)
	,	serverName(serverName)
	,	serviceName(serviceName)
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
	std::string componentName = "PushClient";
	if(argc > 1) {
		componentName = argv[1];
	}
	std::string serverName = "PushServer";
	std::string serviceName = "TextService";

	ClientApplication application(componentName, serverName, serviceName);
	application.run();
	return 0;
}

