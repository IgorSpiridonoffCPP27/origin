#pragma once
#include <iostream>
#include <exception>
using namespace std;

class MyException : public domain_error {
public: 
	using domain_error::domain_error;
	MyException(const string& _Message);
	
	
};

