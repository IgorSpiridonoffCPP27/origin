#include "SquareChetirehygolnic.h"
#include "EXception.h"
SquareChetirehygolnic::SquareChetirehygolnic(int a, int b, int c, int d, int A, int B, int C, int D) :Chetirehygolnic(a, a, a, a, 90, 90, 90, 90) {
    if (a != c && c != b && b != d && A != 90 && B != 90 && C != 90 && D != 90) {
        throw MyException("все стороны не равны, все углы не равны 90");
    }
    name = " вадрат";
}