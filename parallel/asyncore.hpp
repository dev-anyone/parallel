#ifndef ASYNCORE_HPP
#define ASYNCORE_HPP

#include <functional>
#include <atomic>
#include <algorithm>
#include <queue>
#include <deque>

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/mpl/list.hpp>
#include <boost/any.hpp>

namespace parallel { namespace asyncore {

// _master base classes are to avoid recursive inclusion
class _master_task {
public:
    _master_task() {}
    ~_master_task() {}
    void execute() { std::cout << "Trying to call _master_task.execute() - Fix your code to call Task<T>.execute()" << std::endl; }
};

class Task {// : public _master_task{
public:
    Task() {} // Empty constructor for _master_dispatcher
    Task(std::function<boost::any()> act, std::function<void(boost::any)> clb) : action(act), callback(clb) {} // Normal constructor
    Task(Task const& other) { // Copy constructor
        action = other.action;
        callback = other.callback;
    }
    void execute() {
        callback(action());
    }
    //Task<T>* clone() const { return new Task<T>(*this); }

    std::function<boost::any()> action;
    std::function<void(boost::any)> callback;
};

class _master_dispatcher {
public:
    std::queue<_master_task*> tasks;
    std::atomic<int> qsize;
    std::atomic<bool> quit;
    _master_dispatcher() {}
    _master_dispatcher(int thread_count, int worker_sleep_ms) {}
    ~_master_dispatcher() {}
    void post(Task *t) {}
    boost::any next_task() {return _master_task();}
};

class TaskWorker {
private:
    boost::shared_ptr<_master_dispatcher> _dispatcher;
    int _sleep;
    int _id;
public:
    TaskWorker(boost::shared_ptr<_master_dispatcher> d, int sleep_ms, int id) : _sleep(sleep_ms), _id(id), _dispatcher(d) {std::cout << "Init" << std::endl;}
    void operator()() {
        while (!((*_dispatcher).quit))
        {
            try {
                boost::any t = (*_dispatcher).next_task();
                try {
                    Task task = boost::any_cast<Task>(t);
                    task.execute();
                }
                catch (...) {
                    std::cerr << "Error casting from boost::any to Task" << std::endl;
                    abort();
                }
            }
            catch (...) {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(_sleep));
            }
        }
        _dispatcher.reset(); // remove the keepalive shared_ptr
        delete this;
    }
};



class TaskDispatcher : public _master_dispatcher, public boost::enable_shared_from_this<TaskDispatcher> {
public:
    std::queue<boost::any> tasks;
    std::atomic<int> qsize;
    std::atomic<bool> quit;
    int tc, wsms;
    TaskDispatcher() { throw; }
    TaskDispatcher(int thread_count, int worker_sleep_ms) : tc(thread_count), wsms(worker_sleep_ms) {
        qsize.store(0);
        quit.store(false);
    }
    ~TaskDispatcher() {
        quit = true;
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

    void post(Task t)
    {
        qsize++;
        tasks.push(t);
    }

    boost::any next_task()
    {
        if (qsize.load() > 0)
        {
            qsize--;
            boost::any t = tasks.front();
            tasks.pop();
            return t;
        }
        else
        {
            throw; // Handled in TaskWorker::operator()() (see worker.hpp)
        }
    }
};


}}

#endif // ASYNCORE_HPP
