/*
 * CommTextDDS.cpp
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#include "CommTextDDS.h"

using namespace dds::core::xtypes;

template<>
StructType dds_type<CommExampleObjects::CommText>()
{
	StructType dynamic_dds_type("CommExampleObjects::CommText");
	dynamic_dds_type.add_member(Member("text", StringType(512)));
    return dynamic_dds_type;
}

DynamicData serialize(const CommExampleObjects::CommText &object)
{
	DynamicData data(dds_type<CommExampleObjects::CommText>());

	data.value<std::string>("text", object.text);

	return data;
}

void convert(const DynamicData &data, CommExampleObjects::CommText &object)
{
	object.text = data.value<std::string>("text");
}
