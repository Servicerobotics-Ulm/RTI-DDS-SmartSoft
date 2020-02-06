/*
 * CommPose6d.h
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#ifndef EXAMPLES_COMMPOSE6D_H_
#define EXAMPLES_COMMPOSE6D_H_

#include "CommPosition.h"
#include "CommOrientation.h"

namespace CommExampleObjects {

struct CommPose6d {
	CommPosition position;
	CommOrientation orientation;
};


} /* namespace CommExampleObjects */

std::ostream& operator<<(std::ostream &os, const CommExampleObjects::CommPose6d &obj);

#endif /* EXAMPLES_COMMPOSE6D_H_ */
