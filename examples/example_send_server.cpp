/*
 * example_send_serbver.cpp
 *
 *  Created on: Jun 6, 2019
 *      Author: alexej
 */

#include "CommText.h"
#include "CommTextDDS.h"

#include "RtiDdsSmartSoft/SendServerPattern.h"

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


