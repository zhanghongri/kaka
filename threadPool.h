#include <thread>
#include <mutex>
#include <functional>
#include <string>
#include <condition_variable>
#include <deque>
#include <vector>
#include <memory>

#include <future>

namespace RTDF
{
    class nocopyable
    {
        private:
            nocopyable (const nocopyable& x) = delete;
            nocopyable& operator= (const nocopyable&x) = delete;
        public:
            nocopyable() = default;
            ~nocopyable() = default;
    };
    
    class threadPool : public nocopyable
    {
        public:
            typedef std::function<void() > Task;
            
            explicit threadPool (const std::string &name = std::string());
            ~threadPool();
            
            
            void start (int numThreads = 4); //设置线程数，创建numThreads个线程
            void stop();//线程池结束
            void addTask (const Task& f); //任务f在线程池中运行
            void setMaxQueueSize (int maxSize) { _maxQueueSize = maxSize; }  //设置任务队列可存放最大任务数
            template <class Func, class ...Args>
            auto commitTask (Func f, Args&&...args) ->std::future<decltype (f (args...)) >
            {
                using RetType = decltype (f (args...));
                auto task = std::make_shared<std::packaged_task<RetType() >> (std::bind (std::forward<Func> (f),
                            std::forward<Args>args...));
                unique_lock<mutex> lock (_mutex);
                
                while (isFull())
                {
                    _notFull.wait (lock);
                }
                
                assert (!isFull());
                _queue.push_back ([task]() { (*task) (); });
                _notEmpty.notify_one();
                std::future<RetType> fut = task->get_future();
                return fut;
            }
        private:
            bool isFull();//任务队列是否已满
            void runInThread();//线程池中每个thread执行的function
            Task take();//从任务队列中取出一个任务
        private:
            std::mutex _mutex;
            std::condition_variable _notEmpty;
            std::condition_variable _notFull;
            std::string _name;
            std::vector<std::thread> _threads;
            std::deque<Task> _queue;
            size_t _maxQueueSize;
            bool _running;
    };
    
}

