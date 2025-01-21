

#include <iostream>
using namespace std;


#define MODE 1
#ifndef MODE
#error "необходимо определить MODE"


#else
#if MODE ==1
int add(int a, int b) {
	return a + b;
}
#endif
int main()
{
	setlocale(LC_ALL, "ru");
#if MODE==0
	cout << "Работаю в режиме тренировки";
#elif MODE==1
	cout << "Работаю в боевом режиме"<<endl;
	int a, b;
	cout << "Введите число 1: ";
	cin >> a;
	cout << endl<< "Введите число 2: ";
	cin >> b;
	cout << endl<< "Результат сложения: "<<add(a,b);
#else
	cout << "Неизвестный режим. Завершение работы";
#endif 
}

#endif 