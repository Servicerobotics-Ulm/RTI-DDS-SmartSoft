/*
 * CommText.h
 *
 *  Created on: Jun 26, 2019
 *      Author: alexej
 */

#ifndef EXAMPLES_COMMTEXT_H_
#define EXAMPLES_COMMTEXT_H_

#include <string>
#include <iostream>

namespace CommExampleObjects {

struct CommText {
	std::string text;
};

} /* namespace CommExampleObjects */

std::ostream& operator<<(std::ostream &os, const CommExampleObjects::CommText &obj);

#endif /* EXAMPLES_COMMTEXT_H_ */
