#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
using namespace std;


class big_integer {
	int   num_str = 0;
	char* str_num;
public:
	big_integer(const char* str) {
		size_t len = strlen(str);
		str_num = new char[len + 1];  

		strcpy_s(str_num, len + 1, str);
		
		for (int i = 0;i < strlen(str_num);i++) {
			num_str = num_str + (static_cast<int>(str_num[i]) - 48) * pow(10, strlen(str_num)-i-1);
		}

		
	}
	~big_integer(){
		delete[] str_num;
	}

	big_integer(const big_integer& other) : big_integer(other.str_num) {

	}
	big_integer(big_integer&& other) noexcept{
		str_num = other.str_num;
		num_str = other.num_str;
		other.str_num = nullptr;
	}
	big_integer& operator=(const big_integer& other) {
		if (&other != this) {
			delete[] str_num;
			big_integer(other.str_num);
		}
		return *this;
	}
	big_integer& operator=(big_integer&& other)noexcept {
		if (&other != this) {
			delete[] str_num;
			str_num = other.str_num;
			delete other.str_num;
		}
		return *this;
	}

	void printclass() const{
		cout << num_str << endl;
	}
	int get() const {
		return num_str;
	}
	int operator+(const big_integer num2)const {
		return num_str + num2.get();
	}
	int operator*(const big_integer num2) const{
		return num_str * num2.get();
	}

};



int main()
{

	auto num1 = big_integer("1234567");
	auto num2 = big_integer("567");

	int num3 = num1 + num2;
	auto num4 = num1 * num2;

	num1.printclass();
	num2.printclass();

	cout << num3 << endl;
	cout << num4;

	return 0;
}
