/*
 * example_event_client.cpp
 *
 *  Created on: Jul 2, 2019
 *      Author: alexej
 */

#include "CommPose6d.h"
#include "CommPose6dDDS.h"

#include "RtiDdsSmartSoft/EventClientPattern.h"

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
