#include "Counter.h"
#include <iostream>

Counter::Counter(int count){
    this->count = count;
    }
void Counter::add_count() {
    count++;
}
void Counter::minus_count() {
    count--;
}
void Counter::print_count() {
    std::cout << count << std::endl;
}