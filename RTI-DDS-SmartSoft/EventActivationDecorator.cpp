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

#include "RTI-DDS-SmartSoft/EventActivationDecorator.h"

namespace SmartDDS {

using namespace dds::core::xtypes;

EventActivationDecorator::EventActivationDecorator(const StructType &original_dds_type)
:	decorated_dds_type(original_dds_type.name()+"::EventActivation")
{
	EnumType activationModeEnum("EventActivationMode");
	activationModeEnum.add_member(EnumMember("ACTIVATE_SINGLE", static_cast<int>(EventActivationMode::ACTIVATE_SINGLE)));
	activationModeEnum.add_member(EnumMember("ACTIVATE_CONTINUOUS", static_cast<int>(EventActivationMode::ACTIVATE_CONTINUOUS)));
	activationModeEnum.add_member(EnumMember("DEACTIVATE", static_cast<int>(EventActivationMode::DEACTIVATE)));

	decorated_dds_type.add_member(Member("event_activation_mode", activationModeEnum));
	decorated_dds_type.add_member(Member("event_activation_parameters", original_dds_type).optional(true));
}

StructType EventActivationDecorator::getDecoratedDDSType() const
{
	return decorated_dds_type;
}
DynamicData EventActivationDecorator::createDecoratedObject(const DynamicData &original_object, const EventActivationMode &mode) const
{
	DynamicData decorated_object(decorated_dds_type);

	decorated_object.value("event_activation_mode", static_cast<int>(mode));
	decorated_object.value("event_activation_parameters", original_object);

	return decorated_object;
}

dds::core::xtypes::DynamicData EventActivationDecorator::createDeactivationObject() const
{
	DynamicData decorated_object(decorated_dds_type);

	decorated_object.value("event_activation_mode", static_cast<int>(EventActivationMode::DEACTIVATE));

	return decorated_object;
}

DynamicData EventActivationDecorator::extractOriginalObject(const DynamicData &decorated_object) const
{
	return decorated_object.value<DynamicData>("event_activation_parameters");
}
EventActivationMode EventActivationDecorator::extractActivationMode(const dds::core::xtypes::DynamicData &decorated_object) const
{
	return static_cast<EventActivationMode>( decorated_object.value<int>("event_activation_mode") );
}

} /* namespace SmartDDS */
