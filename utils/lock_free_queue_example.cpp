#include "utils.hpp"
#include "lock_free_queue.hpp"

struct MyStruct {
    int d_[3];
};

using namespace kse::utils;

auto consumeFunction(lock_free_queue<MyStruct>* lfq) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(5s);

    while (lfq->size()) {
        const auto d = lfq->get_next_read_element();
        lfq->next_read_index();

        std::cout << "consumeFunction read elem:" << d->d_[0] << "," << d->d_[1] << "," << d->d_[2] << " lfq-size:" << lfq->size() << std::endl;

        std::this_thread::sleep_for(1s);
    }

    std::cout << "consumeFunction exiting." << std::endl;
}

int main() {
    lock_free_queue<MyStruct> lfq(20);

    auto ct = create_thread(-1, consumeFunction, &lfq);

    for (auto i = 0; i < 50; ++i) {
        const MyStruct d{ i, i * 10, i * 100 };
        *(lfq.get_next_write_element()) = d;
        lfq.next_write_index();

        std::cout << "main constructed elem:" << d.d_[0] << "," << d.d_[1] << "," << d.d_[2] << " lfq-size:" << lfq.size() << std::endl;

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    }

    ct.join();

    std::cout << "main exiting." << std::endl;

    return 0;
}