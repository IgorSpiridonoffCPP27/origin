#include <iostream>
#include <exception>
using namespace std;

class smart_array {
	int* arr;
	int Size;
	int i = 0;
public:
	smart_array(int N) {
		Size = N;
		arr = new int[Size];
	}
	void add_element(int a) {
		
		if (i < Size) {
			arr[i] = a;
			i++;
		}
		else {
			throw exception("end memory");
		}
	}
	int get_element(int i2) {
		if (i2 < Size) {
			return arr[i2];
		}
		else {
			throw exception("index out of range");
		}
	}
	smart_array& operator=(smart_array& new_arr) {
		Size = new_arr.Size;
		delete[] arr;
		arr = new int[Size];
		for (int i = 0; i < Size; i++) {
			arr[i] = new_arr.arr[i];
		}
		return *this;
	}
	void print() {
		for (int i = 0; i < Size;i++) {
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
