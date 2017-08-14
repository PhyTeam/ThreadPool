#include <thread>
#include <stdio.h>
#include <queue>

#ifndef THREAD_SAFE_QUEUE
#define THREAD_SAFE_QUEUE

class join_threads
{
private:
    std::vector<std::thread>& _ref;
public:
    join_threads(std::vector<std::thread>& ref): _ref(ref) {}
    ~join_threads()
    {
        std::cout << "Join threads" << std::endl;
        for (auto it  = _ref.begin(); it != _ref.end(); it++) {
            if ((*it).joinable()) {
                (*it).join();
            }
        }
    }
};


using namespace std;

struct queue_empty : std::exception
{
    const char* what() const throw();
};

template <typename T>
class threadsafe_queue {
private:
    std::queue<T> _data;
    std::mutex _mutex;
    std::condition_variable _conditional_var;
public:
    threadsafe_queue() {}
    threadsafe_queue(const threadsafe_queue& other)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _data = other._data;
    }
    threadsafe_queue& operator=(const threadsafe_queue&) = delete;

    void push(T new_value)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _data.push(new_value);
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        if (_data.empty()) return false;

        value = std::move(_data.front());
        _data.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> guard(_mutex);
        if (_data.empty()) throw queue_empty();

        auto ret = std::make_shared<T>(std::move(_data.front()));
        _data.pop();
        return ret;
    }

    bool empty()
    {
        std::lock_guard<std::mutex> guard(_mutex);
        return _data.empty();
    }

};

#endif
