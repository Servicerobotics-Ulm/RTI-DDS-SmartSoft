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

#include "RtiDdsTestingEnvironment.h"

DEFINE_TESTING_ENVIRONMENT(RtiDdsTestingEnvironment);

// DDS serialization of the CommTrajectoryDDS
#include "CommTestObjectsDDS/CommTrajectoryDDS.h"

// DDS component realization
#include "RTI-DDS-SmartSoft/Component.h"

// DDS pattern headers
#include "RTI-DDS-SmartSoft/PushClientPattern.h"
#include "RTI-DDS-SmartSoft/PushServerPattern.h"

#include "RTI-DDS-SmartSoft/SendClientPattern.h"
#include "RTI-DDS-SmartSoft/SendServerPattern.h"

#include "RTI-DDS-SmartSoft/QueryClientPattern.h"
#include "RTI-DDS-SmartSoft/QueryServerPattern.h"

#include "RTI-DDS-SmartSoft/EventClientPattern.h"
#include "RTI-DDS-SmartSoft/EventServerPattern.h"

IComponentPtrType RtiDdsTestingEnvironment::createComponent(const std::string &name) {
	return std::make_shared<SmartDDS::Component>(name);
}

IPushClientPtrType RtiDdsTestingEnvironment::createPushClient()
{
	auto dds_component = std::dynamic_pointer_cast<SmartDDS::Component>(component);
	return IPushClientPtrType(new SmartDDS::PushClientPattern<DataType>(dds_component.get()));
}
IPushServerPtrType RtiDdsTestingEnvironment::createPushServer(const std::string &name)
{
	auto dds_component = std::dynamic_pointer_cast<SmartDDS::Component>(component);
	return IPushServerPtrType(new SmartDDS::PushServerPattern<DataType>(dds_component.get(), name));
}

ISendClientPtrType RtiDdsTestingEnvironment::createSendClient()
{
	auto dds_component = std::dynamic_pointer_cast<SmartDDS::Component>(component);
	return ISendClientPtrType(new SmartDDS::SendClientPattern<DataType>(dds_component.get()));
}
ISendServerPtrType RtiDdsTestingEnvironment::createSendServer(const std::string &name, ISendServerHandlerPtrType handler)
{
	auto dds_component = std::dynamic_pointer_cast<SmartDDS::Component>(component);
	return ISendServerPtrType(new SmartDDS::SendServerPattern<DataType>(dds_component.get(), name, handler));
}

IQueryClientPtrType RtiDdsTestingEnvironment::createQueryClient()
{
	auto dds_component = std::dynamic_pointer_cast<SmartDDS::Component>(component);
	return IQueryClientPtrType(new SmartDDS::QueryClientPattern<RequestType,AnswerType>(dds_component.get()));
}
IQueryServerPtrType RtiDdsTestingEnvironment::createQueryServer(const std::string &name, IQueryServerHandlerPtrType handler)
{
	auto dds_component = std::dynamic_pointer_cast<SmartDDS::Component>(component);
	return IQueryServerPtrType(new SmartDDS::QueryServerPattern<RequestType,AnswerType>(dds_component.get(), name, handler));
}

IEventClientPtrType RtiDdsTestingEnvironment::createEventClient()
{
	auto dds_component = std::dynamic_pointer_cast<SmartDDS::Component>(component);
	return IEventClientPtrType(new SmartDDS::EventClientPattern<ActivationType,EventType>(dds_component.get()));
}
IEventServerPtrType RtiDdsTestingEnvironment::createEventServer(const std::string &name, IEventTestHandlerPtrType handler)
{
	auto dds_component = std::dynamic_pointer_cast<SmartDDS::Component>(component);
	return IEventServerPtrType(new SmartDDS::EventServerPattern<ActivationType,EventType>(dds_component.get(), name, handler));
}
