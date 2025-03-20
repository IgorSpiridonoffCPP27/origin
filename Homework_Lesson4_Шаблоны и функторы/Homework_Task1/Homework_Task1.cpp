#include <iostream>
#include <exception>
#include <vector>
using namespace std;

template<class T>
T square(T a) {
	return a * a;
}
template<>
vector<int> square(vector<int> a2) {
	for (int i = 0;i < a2.size();i++) {
		a2[i]=a2[i]*a2[i];
	}
	return a2;
}


int main()
{
	int a=4;
	cout << square(a) << endl;
	vector<int> a2 = { -1, 4, 8 };
	a2 = square(a2);
	for (int i = 0;i < a2.size();i++) {
		cout << a2[i]<<", ";
	}
	

	return 0;
}
