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

#ifndef RTIDDSSMARTSOFT_COMPONENT_HH_
#define RTIDDSSMARTSOFT_COMPONENT_HH_

#include <smartIComponent.h>

#include "RtiDdsSmartSoft/TimerManagerThread.h"
#include "RtiDdsSmartSoft/DDSInfrastructure.h"

namespace SmartDDS {

class Component : public Smart::IComponent {
private:
	TimerManagerThread timerManager;

	DDSInfrastructure dds_infrastructure;

	// at least a component name needs to be provided so we delete the default constructor
	Component() = delete;

	// objects this class are not supposed to be copied
	Component(const Component &) = delete;
	Component& operator=(const Component &) = delete;

public:
	/** Default constructor initializes the component with a given component-name.
	 *
	 *   @param componentName  unique name of the whole component, which is used by the clients to
	 *                         address this server
	 */
	Component(const std::string &componentName, const int domainId = 0);

	/** Destructor.
	 *
	 */
	virtual ~Component() = default;


	inline DDSInfrastructure& DDS() {
		return dds_infrastructure;
	}


	/** Runs the SmartSoft framework within a component which includes handling
	 *  intercomponent communication etc. This method is called in the main()-routine
	 *  of a component after all initializations including activation of user threads
	 *  are finished. Thypically the last code-line of the man function
	 *  looks like this: "return component.run();".
	 *
	 *  @return status code
	 *    - SMART_ERROR_UNKNOWN: unknown error (probably a corba problem)
	 *    - SMART_OK: gracefully terminated
	 */
	virtual Smart::StatusCode run(void) override;

	virtual void signal_shutdown(void) override;

	/** Allow or abort and reject blocking calls in communication patterns of this component.
	 *
	 *  If blocking is set to false all blocking calls of all communication patterns
	 *  of this component return with SMART_CANCELLED. This can be used to abort blocking
	 *  calls of ALL communication patterns inside a component.
	 *
	 *  @param b (blocking)  true/false
	 *
	 *  @return status code
	 *    - SMART_OK    : new mode set
	 *    - SMART_ERROR : something went wrong
	 */
	virtual Smart::StatusCode blocking(const bool b) override;

	/** get timer-manager for registering timer-handlers
	 *
	 *  An instance of an ITimerManager is instantiated by an IComponent.
	 *  An ITimerManager allows activation of ITimerHandler instances
	 *  that are triggered (once or repeatedly) after a given time period.
	 *
	 *  @return a pointer to the ITimerManager
	 */
	virtual Smart::ITimerManager* getTimerManager() override;
};

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_COMPONENT_HH_ */
