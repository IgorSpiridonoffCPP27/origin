
#include <iostream>
using namespace std;

struct Person
{
    int number;
    string name;
    int money;

};

void new_money(Person* new_person,int new_balance) {
    new_person->money = new_balance;
}

int main()
{
    setlocale(LC_ALL, "Russian");
    Person new_person;
    int new_balance;
    cout << "Введите номер счёта: ";
    cin >> new_person.number;

    cout << "Введите имя владельца: ";
    cin >> new_person.name;

    cout << "Введите баланс: ";
    cin >> new_person.money;
    cout << "Введите новый баланс: ";
    cin >> new_balance;
    new_money(&new_person,new_balance);
    cout << "Ваш счет:"<<new_person.name<<","<<new_person.number<<","<<new_person.money;
}


