#pragma once
class Counter {
    int count;

public:
    Counter(int count = 1);
    void add_count();
    void minus_count();
    void print_count();

};