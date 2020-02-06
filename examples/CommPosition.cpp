/*
 * CommPosition.cpp
 *
 *  Created on: Jul 2, 2019
 *      Author: alexej
 */

#include "CommPosition.h"

std::ostream& operator<<(std::ostream &os, const CommExampleObjects::CommPosition &obj) {
	os << "CommExampleObjects::CommPosition( x=" << obj.x << ", y=" << obj.y << ", z=" << obj.z << " )";
	return os;
}


