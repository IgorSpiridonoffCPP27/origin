#include <iostream>
#include "Lib2.h"
using namespace std;

int main() {
	setlocale(LC_ALL, "ru");
	string a;
	cout << "¬ведите им€: ";
	cin >> a;
	Lib2::Leaver l;
	cout << l.leave(a);



	return 0;
}