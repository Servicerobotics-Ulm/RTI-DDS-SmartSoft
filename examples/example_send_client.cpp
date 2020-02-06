/*
 * example_send_client.cpp
 *
 *  Created on: Jun 6, 2019
 *      Author: alexej
 */

#include "CommText.h"
#include "CommTextDDS.h"

#include "RtiDdsSmartSoft/SendClientPattern.h"

#include <iostream>
#include <thread>
#include <functional>

class ClientApplication {
private:
	SmartDDS::Component component;
	SmartDDS::SendClientPattern<CommExampleObjects::CommText> stringClient;

	std::string serverName;
	std::string serviceName;

	bool cancelled;

	void execute_application() {
		int counter = 0;
		while(!cancelled) {
			CommExampleObjects::CommText obj;
			obj.text = "Hello " + std::to_string(counter++);
			auto status = stringClient.send(obj);
			if(status == Smart::StatusCode::SMART_OK) {
				std::cout << "sent data: " << obj.text << "; sleep(1s)..." << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(1));
			} else {
				std::cout << "trying to connect to " << serverName << "::" << serviceName << "..." << std::endl;
				status = stringClient.connect(serverName, serviceName);
				std::cout << "connection status: " << status << std::endl;
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
	std::string componentName = "SendClient";
	if(argc > 1) {
		componentName = argv[1];
	}
	std::string serverName = "SendServer";
	std::string serviceName = "TextService";

	ClientApplication application(componentName, serverName, serviceName);
	application.run();
	return 0;
}
