#include <iostream>
#include <vector>
#include <future>
#include <algorithm>
#include <iterator>

using namespace std;

void increment(int& x) {
    x += 2;
}


template<typename Iterator, typename Func>
void parallel_for_each(Iterator begin, Iterator end, Func func, size_t min_block_size = 1000) {
    auto distance = std::distance(begin, end);

    if (distance <= static_cast<ptrdiff_t>(min_block_size)) {
        
        std::for_each(begin, end, func);
    }
    else {
        
        Iterator mid = begin;
        std::advance(mid, distance / 2);

        
        auto future = std::async(std::launch::async,
            parallel_for_each<Iterator, Func>, mid, end, func, min_block_size);

        
        parallel_for_each(begin, mid, func, min_block_size);

        
        future.get();
    }
}

int main() {
    std::vector<int> data(10000, 1); 

    parallel_for_each(data.begin(), data.end(), increment);

    
    for (int i = 0; i < 10; ++i)
        std::cout << data[i] << " ";
    std::cout << std::endl;

    return 0;
}
