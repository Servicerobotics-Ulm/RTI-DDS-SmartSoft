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

#include "Comm3dPoseDDS.h"

using namespace dds::core::xtypes;

template<>
StructType dds_type<CommTestObjects::Comm3dPose>()
{
	StructType dynamic_dds_type(CommTestObjects::Comm3dPose::identifier());

	dynamic_dds_type.add_member(Member("x", primitive_type<int32_t>()));
	dynamic_dds_type.add_member(Member("y", primitive_type<int32_t>()));
	dynamic_dds_type.add_member(Member("z", primitive_type<int32_t>()));

    return dynamic_dds_type;
}

DynamicData serialize(const CommTestObjects::Comm3dPose &object)
{
	DynamicData data(dds_type<CommTestObjects::Comm3dPose>());

	data.value<int32_t>("x", object.x);
	data.value<int32_t>("y", object.y);
	data.value<int32_t>("z", object.z);

	return data;
}

void convert(const DynamicData &data, CommTestObjects::Comm3dPose &object)
{
	object.x = data.value<int32_t>("x");
	object.y = data.value<int32_t>("y");
	object.z = data.value<int32_t>("z");
}
