/*
 * CommOrientation.h
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#ifndef EXAMPLES_COMMORIENTATION_H_
#define EXAMPLES_COMMORIENTATION_H_

#include <iostream>

namespace CommExampleObjects {

struct CommOrientation {
	double pitch;
	double yaw;
	double roll;
};

} /* namespace CommExampleObjects */

std::ostream& operator<<(std::ostream &os, const CommExampleObjects::CommOrientation &obj);

#endif /* EXAMPLES_COMMORIENTATION_H_ */
