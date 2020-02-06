/*
 * example_query_server.cpp
 *
 *  Created on: Jun 7, 2019
 *      Author: alexej
 */

#include "CommPose6d.h"
#include "CommPose6dDDS.h"

#include "RtiDdsSmartSoft/QueryServerPattern.h"
#include "RtiDdsSmartSoft/ProcessingPatterns.h"

#include <iostream>

class MyStringHandler
:	public SmartDDS::QueryServerHandler<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d>
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
	  virtual void handleQuery(IQueryServer &server, const Smart::QueryIdPtr &id, const CommExampleObjects::CommPosition& request) override
	  {
		  std::cout << "incoming query request " << request.x << std::endl;

		  CommExampleObjects::CommPose6d response;
		  response.position = request;

		  std::this_thread::sleep_for(std::chrono::seconds(1));
		  auto status = server.answer(id, response);
		  std::cout << "replied " << response.position.x << " with status " << status << std::endl;
	  }
public:
	virtual ~MyStringHandler() = default;
};

using ActiveStringHandler = SmartDDS::ThreadQueueQueryHandler<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d>;

class ServerApplication {
private:
	SmartDDS::Component component;
	std::shared_ptr<MyStringHandler> handler;
	std::shared_ptr<ActiveStringHandler> active_handler;
	SmartDDS::QueryServerPattern<CommExampleObjects::CommPosition,CommExampleObjects::CommPose6d> queryServer;

public:
	ServerApplication(const std::string &componentName, const std::string &serviceName)
	:	component(componentName)
	,	handler(std::make_shared<MyStringHandler>())
	,	active_handler(std::make_shared<ActiveStringHandler>(&component, handler))
	,	queryServer(&component, serviceName, handler)
	{  }

	void run() {
		std::cout << "ServerApplication started" << std::endl;
		component.run();
	}
};

int main(int argc, char* argv[]) {
	std::string componentName = "QueryServer";
	if(argc > 1) {
		componentName = argv[1];
	}
	std::string serviceName = "TextService";

	ServerApplication application(componentName, serviceName);
	application.run();
	return 0;
}


