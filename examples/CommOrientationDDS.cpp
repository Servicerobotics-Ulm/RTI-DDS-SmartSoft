/*
 * CommOrientationDDS.cpp
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#include "CommOrientationDDS.h"

using namespace dds::core::xtypes;

template<>
StructType dds_type<CommExampleObjects::CommOrientation>()
{
	StructType dynamic_dds_type("CommExampleObjects::CommOrientation");
	dynamic_dds_type.add_member(Member("pitch", primitive_type<double>()));
	dynamic_dds_type.add_member(Member("yaw", primitive_type<double>()));
	dynamic_dds_type.add_member(Member("roll", primitive_type<double>()));

    return dynamic_dds_type;
}

DynamicData serialize(const CommExampleObjects::CommOrientation &object)
{
	DynamicData data(dds_type<CommExampleObjects::CommOrientation>());

	data.value<double>("pitch", object.pitch);
	data.value<double>("yaw", object.yaw);
	data.value<double>("roll", object.roll);

	return data;
}

void convert(const DynamicData &data, CommExampleObjects::CommOrientation &object)
{
	object.pitch = data.value<double>("pitch");
	object.yaw = data.value<double>("yaw");
	object.roll = data.value<double>("roll");
}
