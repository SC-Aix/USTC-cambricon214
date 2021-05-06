#ifndef FACE_QUEUE_HPP_
#define FACE_QUEUE_HPP_
#include <mutex>
#include <condition_variable>
#include <queue>

namespace facealign {
template<class T> 
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(ThreadSafeQueue&) =delete;
    
    bool TryPop(T &value);
    bool WaitAndPop(T& value);
    bool WaitAndTryPop(T& value, const std::chrono::microseconds wait_time);
    void Push(const T& new_value);
    
    bool Empty() {
        std::lock_guard<std::mutex> lk(data_mutex_);
        return q_.empty();
    }

    size_t Size() {
        std::lock_guard<std::mutex> lk(data_mutex_);
        return q_.size();
    } 

private:
    std::mutex data_mutex_;
    std::queue<T> q_;
    std::condition_variable arrive_notification_;
};

template<class T> 
bool ThreadSafeQueue<T>::TryPop(T& value) {
    std::unique_lock<std::mutex> lk(data_mutex_);
    if (q_.empty()) {
        value = q_.front();
        q_.pop();
        return true;
    } else {
        return false;
    }
}

template<class T> 
bool ThreadSafeQueue<T>::WaitAndPop(T& value) {
    std::unique_lock<std::mutex> lk(data_mutex_);
    if (arrive_notification_.wait(lk, [&] { return !q_.empty(); })) {
        value = q_.front();
        q_.pop();
        return true;
    } else {
        return false;
    }
}

template<class T> 
bool ThreadSafeQueue<T>::WaitAndTryPop(T& value, const std::chrono::microseconds wait_time) {
    std::unique_lock<std::mutex> lk(data_mutex_);
    if (arrive_notification_.wait_for(lk, wait_time, [&]{ return !q_.empty(); })) {
        value = q_.front();
        q_.pop();
        return true;
    } else {
        return false;
    }
}

template<class T>
void ThreadSafeQueue<T>::Push(const T& new_value) {
    std::unique_lock<std::mutex> lk(data_mutex_);
    q_.push(new_value);
    lk.unlock();
    arrive_notification_.notify_one();
}

}

#endif // !FACE_QUEUE_HPP_#define FACE_QUEUE_HPP_

