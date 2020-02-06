/*
 * CommPosition.h
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#ifndef EXAMPLES_COMMPOSITION_H_
#define EXAMPLES_COMMPOSITION_H_

#include <iostream>

namespace CommExampleObjects {

struct CommPosition {
	double x;
	double y;
	double z;
};

} /* namespace CommExampleObjects */

std::ostream& operator<<(std::ostream &os, const CommExampleObjects::CommPosition &obj);

#endif /* EXAMPLES_COMMPOSITION_H_ */
