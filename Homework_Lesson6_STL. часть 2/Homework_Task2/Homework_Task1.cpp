#include <iostream>
#include <exception>
#include <list> 
#include <set> 
#include <vector> 

using namespace std;



template<class T>
void print_container(T& arr) {
	for (auto el : arr) {
		cout << el << " ";
	}
	cout << endl;

}

int main()
{
	std::set<std::string> test_set = { "one", "two", "three", "four" };
	print_container(test_set); 

	std::list<std::string> test_list = { "one", "two", "three", "four" };
	print_container(test_list); 

	std::vector<std::string> test_vector = { "one", "two", "three", "four" };
	print_container(test_vector); 

	return 0;
}
