#include "PrymoiTrinangle.h"
PrymoiTrinangle::PrymoiTrinangle(int a, int b, int c, int A, int B, int C) :Trinangle(a, b, c, A, B, 90) {
    if (C != 90) {
        cout << "������ � �������� ���� �. ���� � ������� �� 90" << endl;
    }
    name = "������������� �����������";
}