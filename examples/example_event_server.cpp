/*
 * example_event_server.cpp
 *
 *  Created on: Jul 2, 2019
 *      Author: alexej
 */

#include "CommPose6d.h"
#include "CommPose6dDDS.h"

#include "RtiDdsSmartSoft/EventServerPattern.h"

#include <iostream>

class MyEventTestHandler
:	public Smart::IEventTestHandler<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d,CommExampleObjects::CommPose6d>
{
private:
	   virtual bool testEvent(
			   CommExampleObjects::CommPosition& p,
			   CommExampleObjects::CommPose6d& e,
			   const CommExampleObjects::CommPose6d& s) override
	   {
		   e = s;
		   if(e.position.x > p.x) {
			   return true;
		   }
		   return false;
	   }

	   /** This is a hook which is called whenever an event gets activated.
	    *
	    * Each time a client activates an event, this hook is called with the corresponding
	    * parameter. The overloading of this hook is optional. Blocking calls within this
	    * hook should be avoided.
	    *
	    * @param p event activation parameter set
	    */
	   virtual void onActivation(const CommExampleObjects::CommPosition& p) {
	      std::cout << "onActivation(" << p << ")" << std::endl;
	   };
};

class ServerApplication {
private:
	SmartDDS::Component component;
	std::shared_ptr<MyEventTestHandler> handler;
	SmartDDS::EventServerPattern<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d> eventServer;

	bool cancelled;

		void execute_application() {
			int counter = 0;
			Smart::StatusCode status = Smart::StatusCode::SMART_OK;
			while(!cancelled && status == Smart::StatusCode::SMART_OK) {
				CommExampleObjects::CommPose6d pose;
				pose.position.x = counter++;
				status = eventServer.put(pose);
				std::cout << "sent pose " << pose << "; " << status << "; sleep(1000ms)..." << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
		}

public:
	ServerApplication(const std::string &componentName, const std::string &serviceName)
	:	component(componentName)
	,	handler(std::make_shared<MyEventTestHandler>())
	,	eventServer(&component, serviceName, handler)
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
	std::string componentName = "EventServer";
	if(argc > 1) {
		componentName = argv[1];
	}
	std::string serviceName = "TextService";

	ServerApplication application(componentName, serviceName);
	application.run();
	return 0;
}
