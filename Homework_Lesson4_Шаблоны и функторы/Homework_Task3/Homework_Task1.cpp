#include <iostream>
#include <exception>
using namespace std;


class functor_task {
	int s=0;
	int count=0;
public:
	void operator()(int a) {
		if (a % 3 == 0) {
			s += a;
			count++;
		}
	}
	int get_sum() {
		return s;
	}
	int get_count() {
		return count;
	}


};


int main()
{
	functor_task func;
	
	for (int var : { 4, 1, 3, 6, 25, 54 })
	{
		func(var);
	}
	cout << "get_sum() = " << func.get_sum()<<endl;
	cout << "get_sum() = " << func.get_count() << endl;


	return 0;
}
