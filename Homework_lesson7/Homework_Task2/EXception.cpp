#include "EXception.h"

MyException::MyException(const char* message) {
	this->message = message;
}
const char* MyException::what() const {
	return message;
}