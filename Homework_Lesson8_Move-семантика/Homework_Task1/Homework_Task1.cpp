#include <iostream>
#include <vector>
#include <memory>

void move_vectors(std::vector <std::string>& one_cop, std::vector <std::string>& two_cop) {
    one_cop.swap(two_cop);
}

int main()
{
    std::vector <std::string> one = { "test_string1", "test_string2" };
    std::vector <std::string> two;
    move_vectors(one,two);

    for (auto el : two) {
        std::cout << el<<std::endl;
    }
    
    return 0;
}