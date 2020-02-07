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

#include "RTI-DDS-SmartSoft/TimerManagerThread.h"

#include <list>
// using std::bind
#include <functional>

namespace SmartDDS {

TimerManagerThread::TimerManagerThread()
{
	running = false;
	next_timer_id = 0;
}

TimerManagerThread::~TimerManagerThread()
{
	shutdownTimerThread();
}

int TimerManagerThread::startTimerThread()
{
	std::unique_lock<std::mutex> lock(timer_mutex);
	timer_thread = std::thread(std::bind(&TimerManagerThread::timerRunnable, this));
	if(timer_thread.joinable()) {
		running = true;
		return 0;
	}
	return -1;
}

void TimerManagerThread::shutdownTimerThread(const bool &waitUntillCompletion)
{
	// make sure all timers are deleted before the timer thread is put down
	this->deleteAllTimers();

	std::unique_lock<std::mutex> lock(timer_mutex);
	if(running == true) {
		running = false;
		timer_cond.notify_all();
		if(waitUntillCompletion == true && timer_thread.joinable()) {
			lock.unlock();
			timer_thread.join();
		}
	}
}

void TimerManagerThread::timerRunnable()
{
	do {
		std::unique_lock<std::mutex> lock(timer_mutex);

		if(timer_events.empty()) {
			// no timers yet registered -> wait until the first registration
			timer_cond.wait(lock);
		} else {
			// the front element always has the earliest wake-up time (as the timer_events map is sorted)
			auto currentEventIterator = timer_events.cbegin();
			auto currentWakeupTime = currentEventIterator->first;
			auto currentTimerId = currentEventIterator->second;
			if(currentWakeupTime <= Smart::Clock::now()) {
				// we remove the current event from the map as it is now expired
				timer_events.erase(currentEventIterator);

				auto timerEntry = timers[currentTimerId];

				// The timerExpired handler method might block for some time, therefore we release the
				// mutex in order for the timer manager to remain responsive. For instance, this allows
				// to schedule new timers, or to cancel existing timers (including the current one).
				lock.unlock();
				timerEntry.handler->timerExpired(currentWakeupTime, timerEntry.act);
				// please note, the other two handler methods, timerCancelled and timerDeleted
				// are called asynchronously from a different thread!
				lock.lock();

				// the current timer might have been cancelled in the meantime, so we check the id again
				if(timers.find(currentTimerId) != timers.end() && timerEntry.interval > Smart::Duration::zero()) {
					// only if the timer has not yet been cancelled and its interval value is greater than 0
					// then the next wake-up event will be scheduled

					// here it is important not to use the "Clock::now" as a reference for the next interval, but
					// to calculate the next interval relative to the preceding wakupTime!
					auto nextWakupTime = currentWakeupTime + timerEntry.interval;
					// the timer_events map is automatically sorted so that the closest wake-up time is at the front
					timer_events.emplace(nextWakupTime, currentTimerId);
				}
			} else {
				// wait until the current (i.e. the next earliest) wake-up-time is reached
				timer_cond.wait_until(lock, currentWakeupTime);
			}
		}
	} while(running == true);
}

TimerManagerThread::TimerId TimerManagerThread::scheduleTimer(
		Smart::ITimerHandler *handler,
		const void *act,
		const Smart::Duration &oneshot_time,
		const Smart::Duration &interval
	)
{
	std::unique_lock<std::mutex> lock(timer_mutex);

	auto timerId = next_timer_id++;
	// create the first wake-up event (i.e. the one-shot event)
	auto firstWakupTime = Smart::Clock::now() + oneshot_time;
	timer_events.emplace(firstWakupTime, timerId);

	TimerEntry entry;
	entry.handler = handler;
	entry.act = act;
	entry.interval = interval;
	timers[timerId] = entry;

	// trigger timer thread to check the next wake-up time
	timer_cond.notify_all();

	return timerId;
}

int TimerManagerThread::resetTimerInterval(const TimerId& timer_id, const Smart::Duration &interval)
{
	std::unique_lock<std::mutex> lock(timer_mutex);
	auto timerEntry = timers.find(timer_id);
	if(timerEntry != timers.end()) {
		// set the new interval value
		timerEntry->second.interval = interval;

		// erase potentially scheduled events for the given timer-id
		for(auto eventIt = timer_events.cbegin(); eventIt != timer_events.cend(); /* incremented inside */) {
			if(eventIt->second == timer_id) {
				// the next event iterator is returned from the erase method (since C++11)
				eventIt = timer_events.erase(eventIt);
			} else {
				// regular iterator increment
				eventIt++;
			}
		}

		// create a new wake-up event using the new interval value
		// please note, we also reset the start-time to "now" from which the new interval will be scheduled
		auto nextWakupTime = Smart::Clock::now() + interval;
		// the timer_events map is automatically sorted so that the closest wake-up time is at the front
		timer_events.emplace(nextWakupTime, timer_id);

		// trigger the timer thread to reschedule the next valid wake-up event
		timer_cond.notify_all();

		return 0;
	}
	return -1;
}

int TimerManagerThread::cancelTimer(const TimerId& timer_id, const void **act)
{
	std::unique_lock<std::mutex> lock(timer_mutex);
	auto timerIterator = timers.find(timer_id);
	if(timerIterator != timers.end()) {
		// get the timer data copy
		auto timerEntry = timerIterator->second;
		// erase the timer entry
		timers.erase(timerIterator);

		// erase potentially scheduled events for the given timer-id
		for(auto eventIt = timer_events.cbegin(); eventIt != timer_events.cend(); /* incremented inside */) {
			if(eventIt->second == timer_id) {
				// the next event iterator is returned from the erase method (since C++11)
				eventIt = timer_events.erase(eventIt);
			} else {
				// regular iterator increment
				eventIt++;
			}
		}

		// trigger the timer thread to reschedule the next valid wake-up event
		timer_cond.notify_all();

		if(act != nullptr) {
			// return the stored Asynchronous Completion Token (ACT) pointer back (if the act is not null)
			*act = timerEntry.act;
		}

		// Here we call the timerCancelled handler method directly (instead of triggering it from within the
		// timer thread). This is a design choice that mimics the same behavior as the ACE_Timer_Queue, which
		// served as one of the sources for general design considerations. The advantage of this is that the handler
		// will be informed instantly when the cancel timer call happens. The drawback of this is that the timerCancelled
		// will be triggered from a different thread than the timer-thread and thus the handler implementation
		// has to deal with the typical concurrent programming aspects. We release the lock here so that the overall
		// timer manager remains responsive for all the other timers.
		lock.unlock();
		timerEntry.handler->timerCancelled();

		return 0;
	}
	return -1;
}

int TimerManagerThread::cancelTimersOf(Smart::ITimerHandler *handler)
{
	std::unique_lock<std::mutex> lock(timer_mutex);
	// this is the return value
	int numberOfTimersCancelled = 0;

	for(auto timerIterator = timers.cbegin(); timerIterator != timers.cend(); /* incremented inside */) {
		if(timerIterator->second.handler == handler) {
			auto currentTimerId = timerIterator->first;

			numberOfTimersCancelled++;

			// erase and increment timer-iterator
			timerIterator = timers.erase(timerIterator);

			// erase potentially scheduled events for the given timer-id
			for(auto eventIt = timer_events.cbegin(); eventIt != timer_events.cend(); /* incremented inside */) {
				if(eventIt->second == currentTimerId) {
					// the next event iterator is returned from the erase method (since C++11)
					eventIt = timer_events.erase(eventIt);
				} else {
					// regular iterator increment
					eventIt++;
				}
			}

			// trigger the timer thread to reschedule the next valid wake-up event
			timer_cond.notify_all();
		} else {
			// next iterator
			timerIterator++;
		}
	}

	// Here we call the timerCancelled handler method similar to the case when a single timer activation
	// has been cancelled (see above). This behavior defers from the original ACE_Timer_Queue behavior
	// (that served as an inspiration for general design considerations), which calls this method repeatedly,
	// once for each individual timer activation. As the timerCancelled method cannot distinguish the
	// individual timer activations anyways, calling the method repeatedly doesn't seem to make much sense.
	// We release the mutex here again, just for the sake of the timer manager to remain responsive.
	lock.unlock();
	handler->timerCancelled();

	return numberOfTimersCancelled;
}

void TimerManagerThread::deleteAllTimers()
{
	std::unique_lock<std::mutex> lock(timer_mutex);

	// we don't need to check any IDs here so we just clear the whole map
	timer_events.clear();
	// release the timer thread
	timer_cond.notify_all();

	// clone the timers to call timerDeleted handler methods outside of the scoped lock (see below)
	auto timersCopy = timers;
	timers.clear();

	lock.unlock();

	// call all the timerDeleted handler methods outside of the scoped lock
	for(auto timerEntry: timersCopy) {
		timerEntry.second.handler->timerDeleted(timerEntry.second.act);
	}
}

} /* namespace SmartDDS */
