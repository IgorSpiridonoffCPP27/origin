#include "Trinangle.h"
#include "EXception.h"
Trinangle::Trinangle(int a, int b, int c, int A, int B, int C) {
    if ((A + B + C) == 180) {
        name = "Треугольник";
        this->storona_a = a;
        this->storona_b = b;
        this->storona_c = c;
        this->ugol_A = A;
        this->ugol_B = B;
        this->ugol_C = C;
    }
    else {
        throw SumTriAngle();
    }



};
void Trinangle::get_info(){
    cout << name << endl;
    cout << "Стороны: " << " a= " << storona_a << " b= " << storona_b << " c= " << storona_c << endl;
    cout << "Углы: " << " А= " << ugol_A << " B= " << ugol_B << " C= " << ugol_C << endl;
    cout << endl;
};