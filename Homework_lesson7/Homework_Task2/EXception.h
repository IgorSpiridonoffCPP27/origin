#pragma once
#include <iostream>
#include <exception>
using namespace std;

class MyException : public exception {
public: 
	const char* message;
	MyException(const char* message);
	const char* what() const override;
	
	
};

