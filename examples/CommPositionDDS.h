/*
 * CommPositionDDS.h
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#ifndef EXAMPLES_COMMPOSITIONDDS_H_
#define EXAMPLES_COMMPOSITIONDDS_H_

#include <dds/dds.hpp>
#include "CommPosition.h"

// forward declaration
template <typename T>
dds::core::xtypes::StructType dds_type();

// template specialization
template<>
dds::core::xtypes::StructType dds_type<CommExampleObjects::CommPosition>();

dds::core::xtypes::DynamicData serialize(const CommExampleObjects::CommPosition &object);

void convert(const dds::core::xtypes::DynamicData &data, CommExampleObjects::CommPosition &object);

#endif /* EXAMPLES_COMMPOSITIONDDS_H_ */
