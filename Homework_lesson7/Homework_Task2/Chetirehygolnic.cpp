#include "Chetirehygolnic.h"
#include "EXception.h"
Chetirehygolnic::Chetirehygolnic(int a, int b, int c, int d, int A, int B, int C, int D) {
    if ((A + B + C + D) == 360) {
        name = "���������������";
        this->storona_a = a;
        this->storona_b = b;
        this->storona_c = c;
        this->storona_d = d;
        this->ugol_A = A;
        this->ugol_B = B;
        this->ugol_C = C;
        this->ugol_D = D;
    }
    else {
        throw MyException("������ � ����� ����� ����������������");
    }


};
void Chetirehygolnic::get_info() {
    cout << name << endl;
    cout << "�������: " << " a= " << storona_a << " b= " << storona_b << " c= " << storona_c << " d= " << storona_d << endl;
    cout << "����: " << " �= " << ugol_A << " B= " << ugol_B << " C= " << ugol_C << " D= " << ugol_D << endl;
    cout << endl;
};