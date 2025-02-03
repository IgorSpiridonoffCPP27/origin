#include <iostream>
#include <string>
using namespace std;

int main() {
	setlocale(LC_ALL, "ru");

	string a;
	cout << "Введите имя: ";
	cin >> a;
	cout << endl << "Здравствуйте, " << a << endl;


	return 0;
}