#ifndef ASYNCORE_HPP
#define ASYNCORE_HPP

#include <utility>
#include <functional>
#include <atomic>
#include <algorithm>
#include <queue>
#include <deque>

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/any.hpp>

namespace parallel { namespace asyncore {

typedef std::function<void()> voidfn;
typedef std::function<void(unsigned)> callbackfn;

// _master base classes are to avoid recursive inclusion

class Task {
public:
    Task(voidfn&& act, callbackfn&& clb, unsigned taskid) : action(std::forward<voidfn>(act)), callback(std::forward<callbackfn>(clb)), id(taskid) {if (this->callback == nullptr) { std::cerr << "N" << std::endl; throw; } else if (this->action == nullptr) { std::cerr << "N" << std::endl; throw; }} // Normal constructor
    Task(Task const& other) { // Copy constructor for boost::lockfree::queue
        action = other.action;
        callback = other.callback;
        if (this->callback == nullptr) { std::cerr << "N" << std::endl; throw; } else if (this->action == nullptr) { std::cerr << "N" << std::endl; throw; }
    }
    void operator()() {
        if (this->callback == nullptr) { std::cerr << "N" << std::endl; throw; } else if (this->action == nullptr) { std::cerr << "N" << std::endl; throw; }
        this->action();
        this->callback(id);
    }

    voidfn action;
    callbackfn callback;
    unsigned id;
};

class _master_dispatcher {
public:
    boost::lockfree::queue<Task*, boost::lockfree::capacity<1024>> tasks;
    std::atomic<int> qsize;
    std::atomic<bool> quit;
    _master_dispatcher() {}
    _master_dispatcher(int thread_count, int worker_sleep_ms) {}
    ~_master_dispatcher() {}
    void post(Task t) {}
    void next_task(Task *t) {}
};

class TaskWorker {
private:
    boost::shared_ptr<_master_dispatcher> _dispatcher;
    int _sleep;
    int _id;
    int _prev_id;
public:
    TaskWorker(boost::shared_ptr<_master_dispatcher> d, int sleep_ms, int id) : _sleep(sleep_ms), _id(id), _dispatcher(d) {}
    void operator()() {
        while (!((*_dispatcher).quit))
        {
            Task *task;
            try {
                (*_dispatcher).next_task(task);
                if (_prev_id == task->id) { // Do not repeat-execute (bugfix)
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(_sleep));
                    continue;
                }
            }
            catch (...) {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(_sleep));
                continue;
            }
            _prev_id = task->id;
            (*task)();
        }
        _dispatcher.reset(); // remove the keepalive shared_ptr
        delete this;
    }
};



class TaskDispatcher : public _master_dispatcher, public boost::enable_shared_from_this<TaskDispatcher> {
public:
    boost::lockfree::queue<Task*, boost::lockfree::capacity<1024>> tasks; // Fixed size, so no initialization necessary (it would need to happen when declaring the queue, because there is no assignment operator, but this cannot be done because it is a class variable)
    std::atomic<bool> quit;
    int tc, wsms;
    TaskDispatcher() { throw; }
    TaskDispatcher(int thread_count, int worker_sleep_ms) : tc(thread_count), wsms(worker_sleep_ms) {
        quit.store(false);
    }
    ~TaskDispatcher() {
        quit.store(true);
    }

    void init() { // This allows us to call shared_from_this
        // Avoid self-destruction by early-finishing threads
        boost::shared_ptr<TaskDispatcher> t = boost::enable_shared_from_this<TaskDispatcher>::shared_from_this();
        for (int i = 0; i < tc; ++i)
        {
            boost::async(TaskWorker(boost::enable_shared_from_this<TaskDispatcher>::shared_from_this(), wsms, i));
        }
        t.reset();
    }
    void stop() {
        quit = true;
    }

    void post(Task *t)
    {
        if (!tasks.push(t))
        {
            throw; // Error
        }
    }

    void next_task(Task *t)
    {
        if (!tasks.pop(t))
        {
            throw; // Handled by the TaskWorker object asking for a task
        }
    }
};


}}

#endif // ASYNCORE_HPP
