#include <iostream>
#include <exception>
using namespace std;


template<class T>
class array_home {
	int Size=0;
	T* arr = nullptr;
public:

	array_home(int a) :Size(a), arr(new T[Size]) {}
	~array_home() {delete[] arr;}


	T& operator[](int i) {
		return arr[i];
		}
	T& [](int j) {

		}
};


int main()
{
	int row = 5, column = 5;
	int** table = new int*[row];
	for (int i = 0;i < 5;i++) {
		table[i] = new int[column];
	}


	array_home<int> a(5);
	cout<<a[1][2];

	return 0;
}
