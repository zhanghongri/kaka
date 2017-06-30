#include "threadPool.h"
#include <cassert>
using namespace std;

namespace RTDF
{
    threadPool::threadPool (const string &name) :
        _name (name),
        _maxQueueSize (8),
        _running (false)
    {
    }
    
    threadPool::~threadPool()
    {
        if (_running)
        {
            stop();
        }
    }
    
    void threadPool::start (int numThreads)
    {
        assert (_threads.empty());
        _running = true;
        _threads.reserve (numThreads);
        
        for (int i = 0; i < numThreads; ++i)
        {
            _threads.push_back (thread (&threadPool::runInThread, this));
        }
    }
    
    void threadPool::stop()
    {
        {
            unique_lock<mutex>  lock (_mutex);
            _running = false;
            _notEmpty.notify_all();
        }
        
        for (size_t i = 0; i < _threads.size(); ++i)
        {
            _threads[i].join();
        }
    }
    
    void threadPool::addTask (const Task &f)
    {
        unique_lock<mutex> lock (_mutex);
        
        while (isFull())
        {
            _notFull.wait (lock);
        }
        
        assert (!isFull());
        _queue.push_back (f);
        _notEmpty.notify_one();
    }
    
    threadPool::Task threadPool::take()
    {
        unique_lock<mutex> lock (_mutex);
        
        while (_queue.empty() && _running)
        {
            _notEmpty.wait (lock);
        }
        
        Task task;
        
        if (!_queue.empty())
        {
            task = _queue.front();
            _queue.pop_front();
            
            if (_maxQueueSize > 0)
            {
                _notFull.notify_one();
            }
        }
        
        return task;
    }
    
    bool threadPool::isFull()
    {
        return _maxQueueSize > 0 && _queue.size() >= _maxQueueSize;
    }
    
    
    void threadPool::runInThread()
    {
        try
        {
            while (_running)
            {
                //std::this_thread::sleep_for (std::chrono::microseconds (10));
                Task task = take();
                
                if (task)
                {
                    task();
                }
                
                addTask (task);
            }
        }
        
        catch (const exception& ex)
        {
            fprintf (stderr, "exception caught in threadPool %s\n", _name.c_str());
            fprintf (stderr, "reason: %s\n", ex.what());
            abort();
        }
        
        catch (...)
        {
            fprintf (stderr, "exception caught in threadPool %s\n", _name.c_str());
        }
    }
    
}




