/*
 * ProcessingPatterns.h
 *
 *  Created on: Sep 20, 2019
 *      Author: alexej
 */

#ifndef RTIDDSSMARTSOFT_PROCESSINGPATTERNS_H_
#define RTIDDSSMARTSOFT_PROCESSINGPATTERNS_H_

#include <smartIProcessingPatterns_T.h>

#include "RtiDdsSmartSoft/Task.h"
#include "RtiDdsSmartSoft/QueryServerHandler.h"

namespace SmartDDS {

/** Decorator for QueryServerHandler to defer handling to another
 *  thread.
 *
 *  This Decorator (see Design Patterns by Gamma et. al) can be used
 *  if a QueryServerHandler is slow and/or blocking and would
 *  therefore block the framework. This handler is an active object
 *  and has a single thread, in which the Queries are handled one after
 *  another.
 *
 *  example usage:
 *  \code
 *
 *  MySlowQueryHandler: public QueryServerHandler<R,A>
 *  {
 *    ...
 *  };
 *
 *  ...
 *
 *  MySlowQueryHandler slowHandler;
 *  ThreadQueueQueryHandler<R,A> threadedHandler(slowHandler);
 *  QueryServer queryService<R,A>(component,"heavy_computation", threadedHandler);
 *  \endcode
 */
template<class RequestType, class AnswerType>
class ThreadQueueQueryHandler
:	public Smart::IActiveQueryServerHandler<RequestType,AnswerType>
,	public SmartDDS::Task
{
private:
	virtual int task_execution() override {
		return this->process_fifo_queue();
	}
public:
	using IQueryServerHandlerPtr = std::shared_ptr<Smart::IQueryServerHandler<RequestType,AnswerType>>;

	virtual int start() override {
		return SmartDDS::Task::start();
	}

	virtual int stop(const bool wait_till_stopped=true) override
	{
		this->signal_to_stop();
		return SmartDDS::Task::stop();
	}

	/** Create a new threaded QueryServerHandler Decorator.
	 *
	 *  The internal handling thread is started/stopped automatically.
	 *
	 *  @param component          the pointer to the surrounding component
	 *  @param inner_handler_ptr  which will be called in a separate thread.
	 */
	ThreadQueueQueryHandler(Smart::IComponent *component, IQueryServerHandlerPtr inner_handler_ptr)
	:	Smart::IActiveQueryServerHandler<RequestType,AnswerType>(inner_handler_ptr)
	,	SmartDDS::Task(component)
	{
		this->start();
	}

	virtual ~ThreadQueueQueryHandler()
	{
		this->stop();
	}
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_PROCESSINGPATTERNS_H_ */
