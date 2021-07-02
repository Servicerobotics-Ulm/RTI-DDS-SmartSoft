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

#ifndef RTIDDSSMARTSOFT_TASK_H_
#define RTIDDSSMARTSOFT_TASK_H_

#include <atomic>
#include <future>
#include <smartITask.h>

namespace SmartDDS {

class Task : virtual public Smart::ITask {
private:
	std::atomic<bool> cancelled;
	std::future<int> async_handle;
protected:
    /** Tests whether the thread has been signaled to stop.
     *
     * This method allows to implement cooperative thread stopping.
     *
     * @return true if stop was called or false otherwise.
     */
    virtual bool test_canceled() override;

    /** Blocks execution of the calling thread during the span of time specified by rel_time.
     *
     *  Thread-sleeping is sometimes platform-specific. This method encapsulates the
     *  blocking sleep. Calling this method blocks the execution of the calling thread
     *  for a time specified by rel_time.
     *
     *  @param duration relative time duration for the thread to sleep
     */
    virtual void sleep_for(const Smart::Duration &duration) override;

public:
	/// Default constructor
    Task(Smart::IComponent *component = nullptr);

	/// Default destructor
	virtual ~Task() = default;

    /** Creates and starts a new thread (if not yet started)
     *
     *  A new thread is spawned if no thread has been spawned yet before
     *  for this instance.
     *
     *  @return 0 on success (and if thread has already been started) or -1 on failure
     */
    virtual int start() override;

    /** Stops the currently active thread (if it was started before)
     *
     *  The internal thread is signaled to stop. If wait_till_stopped
     *  is set to true then the call to this method blocks until the
     *  internal thread has actually exited (typically using
     *  thread.join() internally).
     *
     *  @param wait_till_stopped waits until the thread has been exited
     *
     *  @return 0 on success or -1 on failure
     */
    virtual int stop(const bool &wait_till_stopped=true) override;
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_TASK_H_ */
