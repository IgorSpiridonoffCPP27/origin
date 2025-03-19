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
		arr.add_element(14);
		arr.add_element(15);
		
		std::cout << arr.get_element(6) << std::endl;
		arr.add_element(15);
		arr.add_element(15);
	}
	catch (const std::exception& ex) {
		std::cout << ex.what() << std::endl;
	}


	return 0;
}
