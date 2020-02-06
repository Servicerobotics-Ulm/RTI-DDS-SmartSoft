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

#ifndef RTIDDSSMARTSOFT_EVENTRESULT_H_
#define RTIDDSSMARTSOFT_EVENTRESULT_H_

#include <list>
#include <mutex>
#include <atomic>

#include <dds/dds.hpp>

#include <smartIEventClientPattern_T.h>

namespace SmartDDS {

template <typename EventType>
class EventResult {
private:
	std::recursive_mutex event_mutex;

	EventType event;
	const Smart::EventMode mode;
	std::atomic<bool> has_new_event;
	dds::core::cond::GuardCondition has_event_guard;
	dds::core::cond::GuardCondition event_deactivated_guard;

	std::list<dds::core::cond::GuardCondition> next_event_guards;
public:
	EventResult(const Smart::EventMode &mode)
	:	event()
	,	mode(mode)
	,	has_new_event(false)
	{  }

	inline bool isSingleMode() const {
		return mode == Smart::EventMode::single;
	}

	inline bool hasNewEvent() const {
		return has_new_event;
	}

	inline void setNewEvent(const EventType &event) {
		std::unique_lock<std::recursive_mutex> event_lock(event_mutex);
		this->event = event;
		has_new_event = true;
		has_event_guard.trigger_value(true);
		for(auto guard: next_event_guards) {
			guard.trigger_value(true);
		}
	}

	inline EventType consumeEvent() {
		std::unique_lock<std::recursive_mutex> event_lock(event_mutex);
		has_new_event = false;
		if(mode == Smart::EventMode::continuous) {
			// only in continuous mode we will reset the guard
			has_event_guard.trigger_value(false);
		}
		return event;
	}

	inline void deactivateEvent() {
		event_deactivated_guard.trigger_value(true);
	}

	const dds::core::cond::GuardCondition& getEventGuard() const {
		return has_event_guard;
	}
	const dds::core::cond::GuardCondition& getDeactivationGuard() const {
		return event_deactivated_guard;
	}

	void addNextEventGuard(const dds::core::cond::GuardCondition &guard) {
		std::unique_lock<std::recursive_mutex> event_lock(event_mutex);
		next_event_guards.push_back(guard);
	}
	inline EventType consumeNextEvent(const dds::core::cond::GuardCondition &guard) {
		std::unique_lock<std::recursive_mutex> event_lock(event_mutex);
		next_event_guards.remove(guard);
		return consumeEvent();
	}
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_EVENTRESULT_H_ */
