/*
 * CommOrientationDDS.h
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#ifndef EXAMPLES_COMMORIENTATIONDDS_H_
#define EXAMPLES_COMMORIENTATIONDDS_H_

#include <dds/dds.hpp>
#include "CommOrientation.h"

// forward declaration
template <typename T>
dds::core::xtypes::StructType dds_type();

// template specialization
template<>
dds::core::xtypes::StructType dds_type<CommExampleObjects::CommOrientation>();

dds::core::xtypes::DynamicData serialize(const CommExampleObjects::CommOrientation &object);

void convert(const dds::core::xtypes::DynamicData &data, CommExampleObjects::CommOrientation &object);

#endif /* EXAMPLES_COMMORIENTATIONDDS_H_ */
