#include <iostream>
#include <exception>
using namespace std;

class smart_array {
	int* arr;
	int Formal_el;
	int Fact_el = 0;
public:
	smart_array(int N) {
		Formal_el = N;
		arr = new int[Formal_el];
	}
	void add_element(int a) {
		
		if (Fact_el < Formal_el) {
			arr[Fact_el] = a;
			Fact_el++;
		}
		else {
			throw exception("end memory");
		}
	}
	int get_element(int i2) {
		if (i2 < Formal_el) {
			return arr[i2];
		}
		else {
			throw exception("index out of range");
		}
	}
	smart_array& operator=(smart_array& new_arr) {
		Formal_el = new_arr.Formal_el;
		Fact_el = new_arr.Fact_el;
		delete[] arr;
		arr = new int[Formal_el];
		for (int i = 0; i < Fact_el; i++) {
			arr[i] = new_arr.arr[i];
		}
		return *this;
	}
	void print() {
		for (int i = 0; i < Fact_el;i++) {
			cout << arr[i] << endl;
		}
	}
	~smart_array() {
		delete[] arr;
	}

};



int main()
{
	try {
		smart_array arr(5);
		arr.add_element(1);
		arr.add_element(4);
		arr.add_element(155);

		smart_array new_array(2);
		new_array.add_element(44);
		new_array.add_element(34);

		
		arr = new_array;

		
		
	}
	catch (const std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}


	return 0;
}
