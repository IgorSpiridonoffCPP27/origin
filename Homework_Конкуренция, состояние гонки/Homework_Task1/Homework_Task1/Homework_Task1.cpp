#include <iostream>
#include <thread>
#include <chrono>
using namespace std;


void CLient(atomic<int>& count, int max_count) {
	for (int i =0;i<max_count;i++) {
		this_thread::sleep_for(1s);
		count++;
		cout << "Пришло, осталось обслужить: " << count << endl;
	}


}

void Operator(atomic<int>&count) {
	while (true) {
		this_thread::sleep_for(2s);
		if (count > 0) {
			count--;
			cout << "Обслужил, осталось: " << count << endl;
		}
		else {
			break;
		}
	}
		

}


int main() {
	setlocale(LC_ALL, "ru");
	cout << "Введите максимальное количество клиентов обрабатываемое оператором: ";

	int max_count;
	atomic<int> count = 0;
	cin >> max_count;
	thread t1(CLient, ref(count), max_count);
	thread t2(Operator,ref(count));
	t1.join();
	t2.join();
	cout << "На сегодня все клиенты обслужены! Их было: "<< max_count;



	return 0;
}