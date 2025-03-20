#include <iostream>
#include <exception>
#include <map>

#include <string>
using namespace std;


int main()
{
	string slovo;
	getline(cin, slovo);
	map<char, int > slovar;
	for (char el : slovo) {
		slovar[el] += 1;
	}
	multimap<int, char> slovar2;
	for (auto& elem : slovar) {
		slovar2.insert({elem.second,elem.first});
	}
	
	for (auto it = slovar2.rbegin(); it!=slovar2.rend();it++)
		std::cout << it->second << ": " << it->first << std::endl;

	
	

	return 0;
}
