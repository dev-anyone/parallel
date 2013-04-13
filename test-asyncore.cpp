#include <iostream>
#include <chrono>
#include <cstdlib>
#include <atomic>
#include <functional>

#include <boost/thread.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

#define SLEEP_ms 25

#include "parallel/asyncore.hpp"

long stepwise_factorial(int of, int step) // Calculates n * (n-step) * (n-2*step) * ... (for step=1, this is the factorial function)
{
    if (of <= 1)
        return 1;
    return of * stepwise_factorial(of-step, step);
}

class callback_provider {
private:
    std::atomic<int> _callbacks_to_go;
    std::atomic<long> _value;
public:
    callback_provider(int how_many) : _callbacks_to_go(how_many) {}
    void callback(boost::any t)
    {
        _value.store(_value.load() * boost::any_cast<long>(t));
        --_callbacks_to_go;
    }
    bool done()
    {
        return (_callbacks_to_go.load() == 0);
    }
    long int value()
    {
        return _value.load();
    }
};

long factorial(int of) // Calculates factorial (n! = n * (n-1) * (n-2) * ... * 2 * 1)
{
    return stepwise_factorial(of, 1);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Not enough arguments" << std::endl;
    }
    int value = std::atoi(argv[1]);

    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point stop;

    std::cout << "Calculating the factorial normally (not parallel) took ";
    start = std::chrono::high_resolution_clock::now(); // Start timing

    long factorial_sync = factorial(value);

    stop = std::chrono::high_resolution_clock::now(); // Stop timing
    std::cout << (std::chrono::microseconds(stop - start).count()) << " µs [microseconds]" << std::endl << "The value is:" << std::endl << factorial_sync << std::endl;

    std::cout << "Calculating the factorial using parallel::asyncore::TaskDispatcher (parallel) took ";
    start = std::chrono::high_resolution_clock::now(); // Start timing

    unsigned tc = boost::thread::hardware_concurrency();
    callback_provider cbp(tc);
    boost::shared_ptr<parallel::asyncore::TaskDispatcher> tdp(new parallel::asyncore::TaskDispatcher(tc, SLEEP_ms));
    tdp->init();
    for (int v = value; v > value-tc && v > 1; --v) // Divide work on threads
    {
        std::function<boost::any()> vl = [v, tc](){return boost::any(stepwise_factorial(v, tc));};
        std::function<void(boost::any)> cb = [&cbp](boost::any value){cbp.callback(value);};
        parallel::asyncore::Task tsk(vl, cb); // Construct task
        tdp->post(tsk); // Post task
    }

    while (!cbp.done())
        boost::this_thread::sleep_for(boost::chrono::milliseconds(SLEEP_ms)); // Wait
    long factorial_async = cbp.value();
    tdp->stop();
    tdp.reset(); // Test shutting down.

    stop = std::chrono::high_resolution_clock::now(); // Stop timing
    std::cout << (std::chrono::microseconds(stop - start).count()) << " µs [microseconds]" << std::endl << "The value is:" << std::endl << factorial_async << std::endl;
    return 0;
}
