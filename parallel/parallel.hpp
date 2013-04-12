#include <boost/thread/thread.hpp>
#include <boost/thread/future.hpp>

#include <algorithm>
#include <utility>
#include <atomic>

#include "mathext.hpp"

#define FS_DEF 115 // Good on 10000000 ints

namespace parallel {

	// Helper functor structs
	
	template <typename Iter, typename F> struct p_for_each_helper_s {
		p_for_each_helper_s(Iter b, Iter em, F f) : begin(b), endmid(em), func(f) {}
		void operator()() {std::for_each(begin, endmid, func);};
		Iter begin;
		Iter endmid;
		F func;
	};
	template <typename Iter, typename F> struct p_for_each_helper_forward_s {
		p_for_each_helper_forward_s(Iter b, Iter em, F&& f) : begin(b), endmid(em), func(std::forward<F>(f)) {}
		void operator()() {std::for_each(begin, endmid, func);};
		Iter begin;
		Iter endmid;
		F func;
	};
	
	template <typename Iter, typename F> struct p_find_helper_s {
	    typedef Iter result_type;
		p_find_helper_s(Iter b, Iter em, Iter d, F f, std::atomic<bool> *fnd, unsigned len, int stp=FS_DEF) : begin(b), endmid(em), end(d), func(f), found(fnd), length(len), FIND_STEP(stp) {}
		Iter operator()() {
			int pcs = FIND_STEP;
			int ct = 0;
			Iter md = begin;
			for (int i = 0; i < ((int)(length / pcs) + 1); ++i)
			{
				ct += pcs;
				if (ct > length)
				{
					pcs = pcs - ct + length; // correct
					if (pcs == 0)
						return end;
				}
				std::advance(md, pcs);
				Iter c = std::find(begin, md, func);
				std::advance(begin, pcs);
				if (found->load() == true)
				{
					return end;
				}
				else if (c != md)
				{
					found->store(true);
					return c;
				}
			}
			return end;
		};
		int FIND_STEP;
		Iter begin;
		Iter endmid;
		Iter end;
		unsigned length;
		std::atomic<bool> *found;
		F func;
	};
	template <typename Iter, typename F> struct p_find_helper_forward_s {
	    typedef Iter result_type;
		p_find_helper_forward_s(Iter b, Iter em, Iter d, F&& f, std::atomic<bool> *fnd, unsigned len, int stp=FS_DEF) : begin(b), endmid(em), end(d), func(std::forward<F>(f)), length(len), found(fnd), FIND_STEP(stp) {}
		Iter operator()() {
			int pcs = FIND_STEP;
			int ct = 0;
			Iter md = begin;
			for (int i = 0; i < ((int)(length / pcs) + 1); ++i)
			{
				ct += pcs;
				if (ct > length)
				{
					pcs = pcs - ct + length; // correct
					if (pcs == 0)
						return end;
				}
				std::advance(md, pcs);
				Iter c = std::find(begin, md, func);
				std::advance(begin, pcs);
				if (found->load() == true)
				{
					return end;
				}
				else if (c != md)
				{
					found->store(true);
					return c;
				}
			}
			return end;
		};
		int FIND_STEP;
		Iter begin;
		Iter endmid;
		Iter end;
		unsigned length;
		std::atomic<bool> *found;
		F func;
	};
	
	template <typename Iter, typename F> struct p_find_first_helper_s {
	    typedef Iter result_type;
		p_find_first_helper_s(Iter b, Iter em, Iter d, F f, std::atomic<int> *fnd, unsigned len, int tid, int stp=FS_DEF) : begin(b), endmid(em), end(d), func(f), found(fnd), length(len), id(tid), FIND_STEP(stp) {}
		Iter operator()() {
			int pcs = FIND_STEP;
			int ct = 0;
			Iter md = begin;
			for (int i = 0; i < ((length / pcs) + 1); ++i)
			{
				ct += pcs;
				if (ct > length)
				{
					pcs = pcs - ct + length; // correct
					if (pcs == 0)
						return end;
				}
				std::advance(md, pcs);
				//std::cout << id << " : " << std::boolalpha << (std::distance(begin, endmid) > std::distance(begin, md)) << std::endl;
				Iter c = std::find(begin, md, func);
				std::advance(begin, pcs);
				if (found->load() < id)
				{
					return end;
				}
				else if (c != md)
				{
					found->store(id);
					return c;
				}
			}
			return end;
		};
		int FIND_STEP;
		Iter begin;
		Iter endmid;
		Iter end;
		unsigned length;
		std::atomic<int> *found;
		int id;
		F func;
	};
	template <typename Iter, typename F> struct p_find_first_helper_forward_s {
	    typedef Iter result_type;
		p_find_first_helper_forward_s(Iter b, Iter em, Iter d, F&& f, std::atomic<int> *fnd, unsigned len, int tid, int stp=FS_DEF) : begin(b), endmid(em), end(d), func(std::forward<F>(f)), length(len), id(tid), found(fnd), FIND_STEP(stp) {}
		Iter operator()() {
			int pcs = FIND_STEP;
			int ct = 0;
			Iter md = begin;
			for (int i = 0; i < ((length / pcs) + 1); ++i)
			{
				ct += pcs;
				if (ct > length)
				{
					pcs = pcs - ct + length; // correct
					if (pcs == 0)
						return end;
				}
				std::advance(md, pcs);
				Iter c = std::find(begin, md, func);
				std::advance(begin, pcs);
				if (found->load() < id)
				{
					return end;
				}
				else if (c != md)
				{
					found->store(id);
					return c;
				}
			}
			return end;
		};
		int FIND_STEP;
		Iter begin;
		Iter endmid;
		Iter end;
		unsigned length;
		std::atomic<int> *found;
		int id;
		F func;
	};
	
	template <typename Seq, typename F>

	// Functions
	
	template <typename Iter, typename F> void p_for_each(Iter begin, Iter end, F&& func) {
		// Applies func to every element between the begin and end iterators
		unsigned threads = boost::thread::hardware_concurrency(); // How many threads to use
		Iter mid = begin;
		int perthread = std::distance(begin, end) / threads; // How large steps per thread
		std::vector<boost::shared_future<void>> results; // Used to wait for the result
		for (int i = 0; i < threads - 1; ++i)
		{
			std::advance(mid, perthread);
			results.push_back(boost::async(p_for_each_helper_s<Iter, F>(begin, mid, func))); // Store "future void" as reference to when it has finished
			std::advance(begin, perthread);
		}
		results.push_back(boost::async(p_for_each_helper_forward_s<Iter, F>(begin, end, std::forward<F>(func)))); // Final thread (not to mid, but to end, to avoid rounding issues), move the functor finally.
		boost::wait_for_all(results.begin(), results.end()); // Wait for finish
	}
	
	template <typename Iter, typename F> Iter p_find_any(Iter begin, Iter end, F func, int step=FS_DEF){
		std::atomic<bool> found(false);
		unsigned threads = boost::thread::hardware_concurrency();
		Iter mid = begin;
		int perthread = std::distance(begin, end) / threads;
		std::vector<boost::shared_future<Iter>> results;
		for (int i = 0; i < threads - 1; ++i)
		{
			std::advance(mid, perthread);
			boost::shared_future<Iter> fu = boost::async(p_find_helper_s<Iter, F>(begin, mid, end, func, &found, perthread, step));
			results.push_back(fu);
			std::advance(begin, perthread);
		}
		boost::shared_future<Iter> fu = boost::async(p_find_helper_forward_s<Iter, F>(begin, end, end, std::forward<F>(func), &found, std::distance(begin, end), step));
		results.push_back(fu); // Final thread
		boost::wait_for_all(results.begin(), results.end()); // Wait for finish
		for (int i = 0; i < threads; ++i)
		{
			Iter e = results[i].get();
			if (e != end) // std::find found a match
				return e;
		}
		return end;
	}
	
	template <typename Iter, typename F> Iter p_find_first(Iter begin, Iter end, F func, int step=FS_DEF){
		unsigned threads = boost::thread::hardware_concurrency();
		std::atomic<int> found(threads);
		Iter mid = begin;
		int perthread = std::distance(begin, end) / threads;
		std::vector<boost::shared_future<Iter>> results;
		for (int i = 0; i < threads - 1; ++i)
		{
			std::advance(mid, perthread);
			boost::shared_future<Iter> fu = boost::async(p_find_first_helper_s<Iter, F>(begin, mid, end, func, &found, perthread, i, step));
			results.push_back(fu);
			std::advance(begin, perthread);
		}
		boost::shared_future<Iter> fu = boost::async(p_find_first_helper_forward_s<Iter, F>(begin, end, end, std::forward<F>(func), &found, std::distance(begin, end), threads-1, step));
		results.push_back(fu); // Final thread
		boost::wait_for_all(results.begin(), results.end()); // Wait for finish
		if (found.load() != threads)
		{
			return results[found.load()].get();
		}
		else
		{
			return end;
		}
	}

	template <typename Seq, typename F> Seq p_sort_allthreads(Seq s, F&& smaller) // uses threads down to the bottom leaf
	{
		int needs_less_threads_than = log2(std::distance(s.begin(), s.end()));
		return p_sort_nthreads(s, std::forward<F>(smaller), needs_less_threads_than);
	}
	
	template <typename Seq, typename F> Seq p_sort_hwthreads(Seq s, F&& smaller) // uses boost::thread::hardware_concurrency() threads, then uses std::sort
	{
		return p_sort_nthreads(s, std::forward<F>(smaller), boost::thread::hardware_concurrency());
	}
	
	template <typename Seq, typename F> Seq p_sort_nthreads(Seq s, F&& smaller, unsigned maxthreadcount)
	{
		// Merge Sort
		typedef Seq::iterator Iter;
		Iter begin = s.begin();
		Iter end = s.end();
		int length = std::distance(begin, end);
		if (maxthreadcount >= 2)
		{
			std::vector<boost::shared_future<Seq>> results;
			boost::shared_future<Seq> fu1 = boost::async(p_
			maxthreadcount -= 2;
		}
	}
	
	// p_sort
	//  parallel std::sort split on k cores -> merge
	//  
	// p_partial_sort
	//  ??
	//
	// p_binary_search
	//  ??
	// Fibonacci parallelisieren
}