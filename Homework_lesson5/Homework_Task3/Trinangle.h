#pragma once
#include "Figure.h"
class Trinangle : public Figure {


public:
    Trinangle(int a, int b, int c, int A, int B, int C);
    void get_info() override;

};