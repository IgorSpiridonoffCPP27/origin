#include <iostream>
#include <exception>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;


int main()
{

	vector<int> arr = { 1,1 ,2 ,5 ,6, 1, 2, 4 };
	sort(arr.begin(),arr.end());
	auto it = unique(arr.begin(), arr.end());
	arr.erase(it, arr.end());
	for (auto el : arr) {
		cout << el<< " ";
	}

	return 0;
}
