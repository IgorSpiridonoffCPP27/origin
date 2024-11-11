
#include <iostream>
using namespace std;

struct Adress {
    string city;
    string ylica;
    int num_home;
    int num_kv;
    int index;
};

void print_adress(Adress gorod) {
    cout << "Город: " << gorod.city << endl;
    cout << "Улица: "<< gorod.ylica << endl;
    cout << "Номер дома: "<< gorod.num_home << endl;
    cout << "Номер квартиры: "<< gorod.num_kv << endl;
    cout << "Индекс: "<< gorod.index << endl;
}

int main()
{
    setlocale(LC_ALL, "Russian");
    
    Adress first,first2;
    
first.city = "Москва";

first.ylica= "Арбат";

first.num_home= 12;

first.num_kv= 8;

first.index= 123456;

print_adress(first);

first2.city = "Ижевск";

first2.ylica = "Пушкина";

first2.num_home = 59;

first2.num_kv = 143;

first2.index = 953769;

print_adress(first2);

}

