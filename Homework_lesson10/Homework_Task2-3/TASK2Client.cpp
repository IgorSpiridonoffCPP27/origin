#include <iostream>
#include "TASK2LIB.h"
using namespace std;

int main() {
	setlocale(LC_ALL, "ru");
	using namespace Task2;
	cout << "¬ведите им€: ";
	string a;
	cin >> a;
	Leaver l;
	cout << l.leave(a);
	return 0;
}