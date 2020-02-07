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

#include "RTI-DDS-SmartSoft/EventServerPattern.h"

#include <iostream>

class MyEventTestHandler
:	public Smart::IEventTestHandler<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d,CommExampleObjects::CommPose6d>
{
public:
	virtual ~MyEventTestHandler() = default;
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
