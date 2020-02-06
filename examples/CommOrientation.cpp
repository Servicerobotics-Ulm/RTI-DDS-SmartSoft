/*
 * CommOrientation.cpp
 *
 *  Created on: Jul 2, 2019
 *      Author: alexej
 */

#include "CommOrientation.h"

std::ostream& operator<<(std::ostream &os, const CommExampleObjects::CommOrientation &obj) {
	os << "CommExampleObjects::CommOrientation( pitch=" << obj.pitch << ", yaw=" << obj.yaw << ", roll=" << obj.roll << " )";
	return os;
}
