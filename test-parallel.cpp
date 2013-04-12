#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdlib>

#include "parallel/parallel.hpp"

#define SIZE 10000000

float randfloat()
{
	return ((float)rand()) / ((float)RAND_MAX) * rand();
}

int main(int argc, char *argv[])
{
	// Inlined parameters:
    //  for_each lambda:     [](float& x){ x *= std::cos(std::tan(std::sin(x-3.1415926536+21)*42)); };
    //  sort lambda:         [](float const& a, float const& b){ return a < b; }
    //  reverse sort lambda: [](float const& a, float const& b){ return a > b; }

	srand(time(0)); // initialize PRNG

    std::cout << "Testing parallel functions compared to their std::... alternatives" << std::endl;
	
	std::vector<float> syncfloatvec(SIZE);
	std::generate(syncfloatvec.begin(), syncfloatvec.end(), randfloat);
	std::vector<float> asyncfloatvec = syncfloatvec;
	
	std::vector<int> syncints(SIZE);
	std::generate(syncints.begin(), syncints.end(), std::rand);
	std::vector<int> asyncints = syncints;
	int searchfor = std::rand();
	
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point stop;
	
	std::chrono::high_resolution_clock::time_point pst;
	std::chrono::high_resolution_clock::time_point psp;
	
    int stp = FS_DEF;


	// normal

	std::cout << "Starting synchronous tasks" << std::endl;
    start = std::chrono::high_resolution_clock::now(); // Start timing
    
    std::cout << "\tTesting std::for_each took ";
    pst = std::chrono::high_resolution_clock::now();
	std::for_each(syncfloatvec.begin(), syncfloatvec.end(), [](float& x){ x *= std::cos(std::tan(std::sin(x-3.1415926536+21)*42)); }); // Do non-async changing
	psp = std::chrono::high_resolution_clock::now();
	std::cout << (std::chrono::nanoseconds(psp - pst).count()/1000) << " µs [microseconds]" << std::endl;
	
    std::cout << "\tTesting std::find took ";
    pst = std::chrono::high_resolution_clock::now();
	std::find(syncints.begin(), syncints.end(), searchfor);
	psp = std::chrono::high_resolution_clock::now();
	std::cout << (std::chrono::nanoseconds(psp - pst).count()/1000) << " µs [microseconds]" << std::endl;
	
    std::cout << "\tTesting std::find as find_any (no find_any available) took ";
    pst = std::chrono::high_resolution_clock::now();
	std::find(syncints.begin(), syncints.end(), searchfor);
	psp = std::chrono::high_resolution_clock::now();
	std::cout << (std::chrono::nanoseconds(psp - pst).count()/1000) << " µs [microseconds]" << std::endl;

    std::cout << "\tTesting std::sort took ";
    pst = std::chrono::high_resolution_clock::now();
    std::sort(syncfloatvec.begin(), syncfloatvec.end(), [](float const& a, float const& b){ return a < b; });
    psp = std::chrono::high_resolution_clock::now();
    std::cout << (std::chrono::nanoseconds(psp - pst).count()/1000) << " µs [microseconds]" << std::endl;
	
    std::cout << "\tTesting std::sort - reversing order took ";
    pst = std::chrono::high_resolution_clock::now();
    std::sort(syncfloatvec.begin(), syncfloatvec.end(), [](float const& a, float const& b){ return a > b; });
    psp = std::chrono::high_resolution_clock::now();
    std::cout << (std::chrono::nanoseconds(psp - pst).count()/1000) << " µs [microseconds]" << std::endl;

    stop = std::chrono::high_resolution_clock::now(); // Stop timing
	std::cout << "Finished synchronous tasks: Time: " << (std::chrono::nanoseconds(stop - start).count()/1000) << "µs [microseconds]" << std::endl;
	
	// parallel
	
	std::cout << "Starting parallel tasks" << std::endl;
	start = std::chrono::high_resolution_clock::now(); // Start timing
	
    std::cout << "\tTesting parallel::p_for_each took ";
    pst = std::chrono::high_resolution_clock::now();
	parallel::p_for_each(asyncfloatvec.begin(), asyncfloatvec.end(), [](float& x){ x *= std::cos(std::tan(std::sin(x-3.1415926536+21)*42)); });
	psp = std::chrono::high_resolution_clock::now();
	std::cout << (std::chrono::nanoseconds(psp - pst).count()/1000) << " µs [microseconds]" << std::endl;
	
    std::cout << "\tTesting parallel::p_find_first took ";
    pst = std::chrono::high_resolution_clock::now();
	parallel::p_find_first(asyncints.begin(), asyncints.end(), searchfor, stp);
	psp = std::chrono::high_resolution_clock::now();
	std::cout << (std::chrono::nanoseconds(psp - pst).count()/1000) << " µs [microseconds]" << std::endl;
	
	std::cout << "\tTesting parallel::p_find_any took ";
    pst = std::chrono::high_resolution_clock::now();
	parallel::p_find_any(asyncints.begin(), asyncints.end(), searchfor, stp);
	psp = std::chrono::high_resolution_clock::now();
	std::cout << (std::chrono::nanoseconds(psp - pst).count()/1000) << " µs [microseconds]" << std::endl;
    
    std::cout << "\tTesting parallel::p_sort_hwthreads took ";
    pst = std::chrono::high_resolution_clock::now();
    parallel::p_sort_hwthreads(asyncfloatvec.begin(), asyncfloatvec.end(), [](float const& a, float const& b){ return a < b; });
    psp = std::chrono::high_resolution_clock::now();
    std::cout << (std::chrono::nanoseconds(psp - pst).count()/1000) << " µs [microseconds]" << std::endl;

    std::cout << "\tTesting parallel::p_sort_allthreads - reversing order took ";
    pst = std::chrono::high_resolution_clock::now();
    parallel::p_sort_allthreads(asyncfloatvec.begin(), asyncfloatvec.end(), [](float const& a, float const& b){ return a > b; });
    psp = std::chrono::high_resolution_clock::now();
    std::cout << (std::chrono::nanoseconds(psp - pst).count()/1000) << " µs [microseconds]" << std::endl;

    stop = std::chrono::high_resolution_clock::now(); // Stop timing
	std::cout << "Finished parallel tasks: Time: " << (std::chrono::nanoseconds(stop - start).count()/1000) << "µs [microseconds]" << std::endl;
	return 0;
}
