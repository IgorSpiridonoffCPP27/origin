#pragma once
#include <iostream>
#include <exception>
using namespace std;

class SumTriAngle : public exception {
public:
	const char* what() const noexcept override { return "Ошибка создания фигуры. Причина: сумма углов не равна 180"; }
};

class AngleC90 : public std::exception {
public:
	const char* what() const override { return "Ошибка создания фигуры. Причина: угол C не равен 90"; }
};

class RB : public std::exception {
public:
	const char* what() const override { return "Ошибка создания фигуры. Причина: стороны a и c не равны, углы A и C не равны"; }
};

class RS : public std::exception {
public:
	const char* what() const override { return "Ошибка создания фигуры. Причина: стороны не равны, все углы не равны 60"; }
};

class SumChetireAngle : public std::exception {
public:
	const char* what() const override { return "Ошибка создания фигуры. Причина: сумма углов не равна 360"; }
};

class Prymoi : public std::exception {
public:
	const char* what() const override { return "Ошибка создания фигуры. Причина: стороны a,c и b,d попарно не равны, все углы не равны 90"; }
};

class KV : public std::exception {
public:
	const char* what() const override { return "Ошибка создания фигуры. Причина: все стороны не равны, все углы не равны 90"; }
};

class Parall : public std::exception {
public:
	const char* what() const override { return "Ошибка создания фигуры. Причина: стороны a,c и b,d попарно не равны, углы A,C и B,D попарно не равны"; }
};

class Romba : public std::exception {
public:
	const char* what() const override { return "Ошибка создания фигуры. Причина: все стороны не равны, углы A,C и B,D попарно не равны"; }
};