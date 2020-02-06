/*
 * example_push_server.cpp
 *
 *  Created on: Jun 5, 2019
 *      Author: alexej
 */

#include "CommPosition.h"
#include "CommPositionDDS.h"

#include "RtiDdsSmartSoft/PushServerPattern.h"

#include <iostream>
#include <thread>
#include <functional>

class ServerApplication {
private:
	SmartDDS::Component component;
	SmartDDS::PushServerPattern<CommExampleObjects::CommPosition> stringServer;

	bool cancelled;

	void execute_application() {
		int counter = 0;
		Smart::StatusCode status = Smart::StatusCode::SMART_OK;
		while(!cancelled && status == Smart::StatusCode::SMART_OK) {
			CommExampleObjects::CommPosition pos;
			pos.x = counter++;
			status = stringServer.put(pos);
			std::cout << "sent pos " << pos.x << "; " << status << "; sleep(10ms)..." << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}

public:
	ServerApplication(const std::string &componentName, const std::string &serviceName)
	:	component(componentName)
	,	stringServer(&component, serviceName, std::chrono::seconds(1))
	,	cancelled(false)
	{  }

	void run() {
		std::thread clientThread = std::thread(std::bind(&ServerApplication::execute_application, this));
		component.run();
		cancelled = true;
		if(clientThread.joinable()) {
			clientThread.join();
		}
	}
};

int main(int argc, char* argv[]) {
	std::string componentName = "PushServer";
	if(argc > 1) {
		componentName = argv[1];
	}
	std::string serviceName = "TextService";

	ServerApplication application(componentName, serviceName);
	application.run();
	return 0;
}
