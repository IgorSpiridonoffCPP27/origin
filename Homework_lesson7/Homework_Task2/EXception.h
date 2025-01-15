#pragma once
#include <iostream>
#include <exception>
using namespace std;

class SumTriAngle : public exception {
public:
	const char* what() const noexcept override { return "������ �������� ������. �������: ����� ����� �� ����� 180"; }
};

class AngleC90 : public std::exception {
public:
	const char* what() const override { return "������ �������� ������. �������: ���� C �� ����� 90"; }
};

class RB : public std::exception {
public:
	const char* what() const override { return "������ �������� ������. �������: ������� a � c �� �����, ���� A � C �� �����"; }
};

class RS : public std::exception {
public:
	const char* what() const override { return "������ �������� ������. �������: ������� �� �����, ��� ���� �� ����� 60"; }
};

class SumChetireAngle : public std::exception {
public:
	const char* what() const override { return "������ �������� ������. �������: ����� ����� �� ����� 360"; }
};

class Prymoi : public std::exception {
public:
	const char* what() const override { return "������ �������� ������. �������: ������� a,c � b,d ������� �� �����, ��� ���� �� ����� 90"; }
};

class KV : public std::exception {
public:
	const char* what() const override { return "������ �������� ������. �������: ��� ������� �� �����, ��� ���� �� ����� 90"; }
};

class Parall : public std::exception {
public:
	const char* what() const override { return "������ �������� ������. �������: ������� a,c � b,d ������� �� �����, ���� A,C � B,D ������� �� �����"; }
};

class Romba : public std::exception {
public:
	const char* what() const override { return "������ �������� ������. �������: ��� ������� �� �����, ���� A,C � B,D ������� �� �����"; }
};