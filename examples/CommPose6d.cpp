/*
 * CommPose6d.cpp
 *
 *  Created on: Jul 2, 2019
 *      Author: alexej
 */

#include "CommPose6d.h"

std::ostream& operator<<(std::ostream &os, const CommExampleObjects::CommPose6d &obj) {
	os << "CommExampleObjects::CommPose6d( position=" << obj.position << ", orientation=" << obj.orientation << " )";
	return os;
}
