
#include <iostream>
#include<vector>
#include<future>
using namespace std;



void search_min_test(int minimum, vector<int>::iterator ykaz_min, vector<int>& a, promise< vector<int>::iterator> PROM) {
    for (auto i = ykaz_min + 1;i < a.end();i++) {
        if (*i < minimum) {
            minimum = *i;
            ykaz_min = i;
        }
    }
    PROM.set_value(ykaz_min);
}

void Choise_Sort_test(vector<int>& a) {
    for (auto j = a.begin();j < a.end() - 1;j++) {
        promise< vector<int>::iterator> PROMISE;
        future< vector<int>::iterator> FUTURE = PROMISE.get_future();
        auto ykaz_min = async(search_min_test,*j, j, ref(a),move(PROMISE));
        swap(*FUTURE.get(), *j);
    }


}


int main()
{

    vector<int> a = { 4,5,9,8,1,3,2};
    Choise_Sort_test(a);

    for (auto i : a) {
        cout << i << endl;
    }



}
