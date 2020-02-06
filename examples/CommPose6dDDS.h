/*
 * CommPose6dDDS.h
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#ifndef EXAMPLES_COMMPOSE6DDDS_H_
#define EXAMPLES_COMMPOSE6DDDS_H_

#include <dds/dds.hpp>

#include "CommPose6d.h"

#include "CommPositionDDS.h"
#include "CommOrientationDDS.h"

// forward declaration
template <typename T>
dds::core::xtypes::StructType dds_type();

// template specialization
template<>
dds::core::xtypes::StructType dds_type<CommExampleObjects::CommPose6d>();

dds::core::xtypes::DynamicData serialize(const CommExampleObjects::CommPose6d &object);

void convert(const dds::core::xtypes::DynamicData &data, CommExampleObjects::CommPose6d &object);

#endif /* EXAMPLES_COMMPOSE6DDDS_H_ */
