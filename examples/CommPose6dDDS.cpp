/*
 * CommPose6dDDS.cpp
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#include "CommPose6dDDS.h"

using namespace dds::core::xtypes;

template<>
StructType dds_type<CommExampleObjects::CommPose6d>()
{
	StructType dynamic_dds_type("CommExampleObjects::CommPose6d");
	dynamic_dds_type.add_member(Member("position", dds_type<CommExampleObjects::CommPosition>()));
	dynamic_dds_type.add_member(Member("orientation", dds_type<CommExampleObjects::CommOrientation>()));

    return dynamic_dds_type;
}

DynamicData serialize(const CommExampleObjects::CommPose6d &object)
{
	DynamicData data(dds_type<CommExampleObjects::CommPose6d>());

	data.value("position", serialize(object.position));
	data.value("orientation", serialize(object.orientation));

	return data;
}

void convert(const DynamicData &data, CommExampleObjects::CommPose6d &object)
{
	convert(data.value<DynamicData>("position"), object.position);
	convert(data.value<DynamicData>("orientation"), object.orientation);
}
