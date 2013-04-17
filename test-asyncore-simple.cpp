#include <iostream>
#include <unistd.h> // sleep(seconds)
#include <utility>
#include <functional>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "parallel/asyncore.hpp"

void taskfn() {
    std::cout << "V" << std::endl;
}

void callback(long int i) {
    std::cout << i << std::endl;
}

using namespace parallel::asyncore;

int main()
{
    unsigned tc = boost::thread::hardware_concurrency();
    boost::shared_ptr<parallel::asyncore::TaskDispatcher> tdp(new parallel::asyncore::TaskDispatcher(tc, 250));
    tdp->init();
    parallel::asyncore::Task tsk(std::forward<voidfn>(voidfn(taskfn)), std::forward<callbackfn>(callbackfn(callback)), (unsigned long) 42); // Construct task
    for (int i = 0; i < tc; ++i) // Divide work on threads
    {
        //std::function<boost::any()> vl = [](){std::cout << "Task-Value-Call" << std::endl; return boost::any(42);};
        //std::function<void(boost::any)> cb = [](boost::any value){std::cout << "Task-Callback-Call" << std::endl; std::cout << boost::any_cast<int>(value) << std::endl;};
        //tdp->post(&Task(std::forward<voidfn>(voidfn(taskfn)), std::forward<callbackfn>(callbackfn(callback)), (unsigned long) i));
        tdp->post(&tsk); // Post task
    }
    sleep(5);
    tdp->stop();
    tdp.reset(); // Test shutting down.
    return 0;
}
