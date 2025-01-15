#pragma once
#include <iostream>
using namespace std;
class Figure {

protected:
    string name = "Фигура";
    int storona_a, storona_b, storona_c, storona_d, ugol_A, ugol_B, ugol_C, ugol_D;
public:

    virtual void get_info();
};
