/*
 * CommPositionDDS.cpp
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#include "CommPositionDDS.h"

using namespace dds::core::xtypes;

template<>
StructType dds_type<CommExampleObjects::CommPosition>()
{
	StructType dynamic_dds_type("CommExampleObjects::CommPosition");
	dynamic_dds_type.add_member(Member("x", primitive_type<double>()));
	dynamic_dds_type.add_member(Member("y", primitive_type<double>()));
	dynamic_dds_type.add_member(Member("z", primitive_type<double>()));

    return dynamic_dds_type;
}

DynamicData serialize(const CommExampleObjects::CommPosition &object)
{
	DynamicData data(dds_type<CommExampleObjects::CommPosition>());

	data.value<double>("x", object.x);
	data.value<double>("y", object.y);
	data.value<double>("z", object.z);

	return data;
}

void convert(const DynamicData &data, CommExampleObjects::CommPosition &object)
{
	object.x = data.value<double>("x");
	object.y = data.value<double>("y");
	object.z = data.value<double>("z");
}
