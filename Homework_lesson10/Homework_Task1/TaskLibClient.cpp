#include <iostream>
#include "Task1Lib.h"
using namespace std;

int main() {
	setlocale(LC_ALL, "ru");
	using namespace TaskLib1;
	string a;
	Greeter str;
	cout << "¬ведите им€: ";
	cin >> a;
	cout << str.greet(a);

	return 0;
}
