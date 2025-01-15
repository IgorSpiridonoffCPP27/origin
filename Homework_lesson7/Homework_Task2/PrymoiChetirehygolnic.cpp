#include "PrymoiChetirehygolnic.h"
#include "EXception.h"
PrymoiChetirehygolnic::PrymoiChetirehygolnic(int a, int b, int c, int d, int A, int B, int C, int D) :Chetirehygolnic(a, a, b, b, 90, 90, 90, 90) {
    if (a != c && b != d || A != 90 && B != 90 && C != 90 && D != 90) {
        throw Prymoi();
    }
    name = "Прямоугольник";
}