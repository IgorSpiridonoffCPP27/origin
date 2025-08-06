#include <iostream>
#include <boost/algorithm/string.hpp>

int main() {
    std::string text = "hello, boost!";
    boost::to_upper(text);
    std::cout << text << std::endl;
    system("pause");
    return 0;
}