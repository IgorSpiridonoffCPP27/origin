#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <iomanip>
#include <mutex>

using namespace std;
using namespace chrono;

once_flag o_flag;

void First(int FPol) {
    cout << "Количество аппаратных ядер - " << thread::hardware_concurrency() << endl << endl;
}

void POTOK(vector<int>& vec1, vector<int>& vec2, vector<int>& vec3, int Pot, int start, int end) {
    call_once(o_flag, First, Pot);
    for (int i = start; i < end; i++) {
        vec3[i] = vec1[i] + vec2[i];
    }
}

double Information(int PotCol, int N) {
    vector<int> v1(N), v2(N), v3(N);
    for (int i = 0; i < N; i++) {
        v1[i] = rand() % 100;
        v2[i] = rand() % 100;
    }

    vector<thread> VT;
    auto start_time = high_resolution_clock::now();

    for (int i = 0; i < PotCol; i++) {
        int start = i * N / PotCol;
        int end = (i == PotCol - 1) ? N : (i + 1) * N / PotCol;
        VT.emplace_back(POTOK, ref(v1), ref(v2), ref(v3), PotCol, start, end);
    }

    for (auto& t : VT) t.join();

    auto end_time = high_resolution_clock::now();
    duration<double> elapsed = end_time - start_time;
    return elapsed.count();
}

int main() {
    setlocale(LC_ALL, "ru");

    vector<int> threads = { 1, 2, 4, 8, 16 };
    vector<int> sizes = { 1000, 10000, 100000, 1000000 };
    vector<vector<double>> results(threads.size(), vector<double>(sizes.size()));

    for (size_t i = 0; i < threads.size(); i++) {
        for (size_t j = 0; j < sizes.size(); j++) {
            results[i][j] = Information(threads[i], sizes[j]);
        }
    }


    
    for (int s : sizes) {
        cout << setw(15) << s;
    }
    cout << endl;

    for (size_t i = 0; i < threads.size(); i++) {
        cout << threads[i] << " потоков";
        for (double t : results[i]) {
            cout << setw(12) << fixed << setprecision(6) << t << "s";
        }
        cout << endl;
    }

    return 0;
}
