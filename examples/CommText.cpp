/*
 * CommText.cpp
 *
 *  Created on: Jul 2, 2019
 *      Author: alexej
 */

#include "CommText.h"

std::ostream& operator<<(std::ostream &os, const CommExampleObjects::CommText &obj) {
	os << "CommExampleObjects::CommText( text=" << obj.text << " )";
	return os;
}
