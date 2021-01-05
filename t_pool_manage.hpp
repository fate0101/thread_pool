#ifndef t_pool_manage_hpp
#define t_pool_manage_hpp


#include "noncopyable.hpp"


#include <assert.h>
#include <string.h>
#include <future>
#include <iostream>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>


namespace n_thread_pool {

class task_pool final : noncopyable
{

public:
    explicit task_pool(unsigned int initCount = 0) noexcept : \
    tCount_(initCount), ths_(nullptr),stop_(false){

    }

    void initThs()
    {
        if (!tCount_) tCount_ = std::thread::hardware_concurrency();

        ths_ = new std::future<void>[tCount_];

        assert(ths_ != nullptr);
        for (unsigned int i = 0; i < tCount_; i++) {

            ths_[i] = std::async(std::launch::async,
                [=]()->void {
                    __thr(this);
                    });
        }
    }

    ~task_pool()
    {
        stop_ = true;
        cv_.notify_all();
        releaseThs();
    }

    void releaseThs()
    {
        assert(tCount_ > 0);
        assert(ths_ != nullptr);

        delete [] ths_;
    }

    void addTask(std::function<void ()>&& f)
    {
        assert(!stop_);
        std::lock_guard<std::mutex> lck(mtx_);
        task_.push(f);

        telescopicThreadCount();
        cv_.notify_one();
    }

    void telescopicThreadCount() noexcept
    {
        // 根据任务数量伸缩线程数，这里伸缩不能太频繁
        // 最好能根据一段时间的任务进行动态调整
        // 最好根据论文或者依据实验数据来编写

        // 线程数量过多会增大 线程切换开销
        // 另外线程可以减少后续任务的等待时间，但是会增加总执行时间
    }

    static void __thr(task_pool* tp)
    {
        for (;;) {
            std::unique_lock <std::mutex> lck(tp->mtx_);
            tp->cv_.wait(lck, [tp_o = tp]() -> bool {
                return !tp_o->task_.empty() || tp_o->stop_;
                });

            if (tp->stop_) break;
            auto f = tp->task_.front();
            tp->task_.pop();
            lck.unlock();

            f();
        };
    }


private:
    unsigned int tCount_;
    std::future<void>* ths_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<std::function<void ()>> task_;
    bool stop_;
};

}  // n_thread_pool
#endif  // t_pool_manage_hpp
