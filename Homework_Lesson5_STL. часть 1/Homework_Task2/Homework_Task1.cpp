#include <iostream>
#include <exception>
#include <list>
using namespace std;





int main()
{
	int N;
	cin >> N;
	list<int> ls;
	for (int i = 0;i < N;i++) {
		int a;
		cin >> a;
		ls.push_front(a);
	}

	ls.unique();
	ls.sort();
	for (auto it = ls.rbegin();it != ls.rend();it++) {
		cout << *it<<endl;
	}

	return 0;
}
