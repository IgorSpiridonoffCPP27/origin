﻿

#include <iostream>

using namespace std;

class Figure {

protected:
    string name = "Фигура: ",logic = "правильная";
    int storona_a, storona_b, storona_c, storona_d, ugol_A, ugol_B, ugol_C, ugol_D,count=0;
public:

    virtual void get_info() {
        cout << name << endl;
        cout << logic << endl;
        cout << "Количество сторон: " <<count<< endl;
        cout << endl;
    }
};

class Trinangle : public Figure {


public:
    Trinangle(int a, int b, int c, int A, int B, int C) {
        count = 3;
        name = "Треугольник:";
        this->storona_a = a;
        this->storona_b = b;
        this->storona_c = c;
        this->ugol_A = A;
        this->ugol_B = B;
        this->ugol_C = C;


    };
    void get_info() override {
        cout << name << endl;
        cout << logic << endl;
        cout << "Количество сторон: " << count << endl;
        cout << "Стороны: " << " a= " << storona_a << " b= " << storona_b << " c= " << storona_c << endl;
        cout << "Углы: " << " А= " << ugol_A << " B= " << ugol_B << " C= " << ugol_C << endl;
        cout << endl;
    };

};

class PrymoiTrinangle : public Trinangle {
public:
    PrymoiTrinangle(int a, int b, int c, int A, int B, int C) :Trinangle(a, b, c, A, B, 90) {
        if (C != 90) {
            logic = "неправильная";
        }
        name = "Прямоугольный треугольник:";
    }

};

class RBTrinangle : public Trinangle {
public:
    RBTrinangle(int a, int b, int c, int A, int B, int C) :Trinangle(a, b, a, A, B, C) {
        if (a != c) {
            logic = "неправильная";
        }
        name = "Равнобедренный треугольник:";
    }
};

class RSTrinangle : public Trinangle {
public:
    RSTrinangle(int a, int b, int c, int A, int B, int C) :Trinangle(a, a, a, 60, 60, 60) {
        if (a != c && b != c || A != 60 && B != 60 && C != 60) {
            logic = "неправильная";
        }
        name = "Равностороний треугольник:";
    }
};

class Chetirehygolnic : public Figure {


public:
    Chetirehygolnic(int a, int b, int c, int d, int A, int B, int C, int D) {
        count = 4;
        name = "Четырехугольник";
        this->storona_a = a;
        this->storona_b = b;
        this->storona_c = c;
        this->storona_d = d;
        this->ugol_A = A;
        this->ugol_B = B;
        this->ugol_C = C;
        this->ugol_D = D;


    };
    void get_info() override {
        cout << name << endl;
        cout << logic << endl;
        cout << "Количество сторон: " << count << endl;
        cout << "Стороны: " << " a= " << storona_a << " b= " << storona_b << " c= " << storona_c << " d= " << storona_d << endl;
        cout << "Углы: " << " А= " << ugol_A << " B= " << ugol_B << " C= " << ugol_C << " D= " << ugol_D << endl;
        cout << endl;
    };

};

class PrymoiChetirehygolnic : public Chetirehygolnic {
public:
    PrymoiChetirehygolnic(int a, int b, int c, int d, int A, int B, int C, int D) :Chetirehygolnic(a, a, b, b, 90, 90, 90, 90) {
        if (a != c && b != d || A != 90 && B != 90 && C != 90 && D != 90) {
            logic = "неправильная";
        }
        name = "Прямоугольник:";
    }

};

class SquareChetirehygolnic : public Chetirehygolnic {
public:
    SquareChetirehygolnic(int a, int b, int c, int d, int A, int B, int C, int D) :Chetirehygolnic(a, a, a, a, 90, 90, 90, 90) {
        if (a != c && c != b && b != d && A != 90 && B != 90 && C != 90 && D != 90) {
            logic = "неправильная";
        }
        name = "Квадрат:";
    }
};

class Parallelogram : public Chetirehygolnic {
public:
    Parallelogram(int a, int b, int c, int d, int A, int B, int C, int D) :Chetirehygolnic(a, a, b, b, A, A, B, B) {
        if (a != c && b != d || A != C && B != D) {
            logic = "неправильная";
        }
        name = "Параллелограмм:";
    }
};
class Romb : public Chetirehygolnic {
public:
    Romb(int a, int b, int c, int d, int A, int B, int C, int D) :Chetirehygolnic(a, b, c, d, A, B, C, D) {
        if (a != c && c != b && b != d || A != C && B != D) {
            logic = "неправильная";
        }
        name = "Ромб:";
    }
};


void print_info(Figure* ptrFigure) {
    ptrFigure->get_info();
}

int main()
{
    setlocale(LC_ALL, "ru");
    Figure fg;
    print_info(&fg);
    Trinangle tr(11, 22, 33, 50, 30, 120);
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
