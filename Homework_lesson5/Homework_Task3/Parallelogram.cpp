#include "Parallelogram.h"
Parallelogram::Parallelogram(int a, int b, int c, int d, int A, int B, int C, int D) :Chetirehygolnic(a, a, b, b, A, A, B, B) {
    if (a != c && b != d || A != C && B != D) {
        cout << "������ � ��������� ����� A � C, B � D. ������ � ��������� ������ � != c � b!=d. ���� ��������. ������� ��������" << endl;
    }
    name = "��������������";
}