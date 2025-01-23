

#include <iostream>
#include "EXception.h"
#include "Figure.h"
#include "Trinangle.h"
#include "PrymoiTrinangle.h"
#include "RBTrinangle.h"
#include "RSTrinangle.h"
#include "Chetirehygolnic.h"
#include "PrymoiChetirehygolnic.h"
#include "SquareChetirehygolnic.h"
#include "Parallelogram.h"
#include "Romb.h"



using namespace std;


void print_info(Figure* ptrFigure) {
    ptrFigure->get_info();
}

int main()
{
    setlocale(LC_ALL, "ru");
    try {
        Trinangle tr(11, 22, 33, 50, 10, 120);
        print_info(&tr);
        PrymoiTrinangle pr(10, 20, 30, 50, 60, 80);
        print_info(&pr);
        RBTrinangle rb(10, 20, 30, 50, 60, 80);
        print_info(&rb);
        RSTrinangle rs(10, 20, 30, 50, 60, 80);
        print_info(&rs);
        Chetirehygolnic ch(11, 22, 33, 44, 50, 30, 120, 80);
        print_info(&ch);
        PrymoiChetirehygolnic pry(11, 22, 33, 44, 50, 30, 120, 80);
        print_info(&pry);
        SquareChetirehygolnic Sq(11, 22, 33, 44, 50, 30, 120, 80);
        print_info(&Sq);
        Parallelogram par(11, 22, 33, 44, 50, 30, 120, 80);
        print_info(&par);
        Romb ro(11, 22, 33, 44, 50, 30, 120, 80);
        print_info(&ro);
    }
    catch (const MyException& a) {
        a.what();
    }
    
}

