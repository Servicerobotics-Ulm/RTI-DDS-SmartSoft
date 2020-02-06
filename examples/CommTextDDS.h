/*
 * CommTextDDS.h
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#ifndef EXAMPLES_COMMTEXTDDS_H_
#define EXAMPLES_COMMTEXTDDS_H_

#include <dds/dds.hpp>
#include "CommText.h"

// forward declaration
template <typename T>
dds::core::xtypes::StructType dds_type();

// template specialization
template<>
dds::core::xtypes::StructType dds_type<CommExampleObjects::CommText>();

dds::core::xtypes::DynamicData serialize(const CommExampleObjects::CommText &object);

void convert(const dds::core::xtypes::DynamicData &data, CommExampleObjects::CommText &object);

#endif /* EXAMPLES_COMMTEXTDDS_H_ */
