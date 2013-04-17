#include <boost/thread/thread.hpp>
#include <boost/thread/future.hpp>

#include <algorithm>
#include <utility>
#include <atomic>

#define FS_DEF 115 // Good on 10000000 ints
#define SIL_DEF 1000 // Good on   "    ints

namespace parallel {

	// Helper functor structs
    //template <typename Iter, typename F> void p_sort_nthreads(Iter begin, Iter end, F smaller, unsigned maxthreadcount); // to allow p_sort_helper_s to call this function
	
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
	
    template <typename Iter, typename F> void p_sort_nthreads(Iter begin, Iter end, F smaller, unsigned maxthreadcount, int SKIP_IF_LESS=SIL_DEF) // SKIP_IF_LESS: Do not spawn threads if the length is smaller
    {
        bool cond_threads = (maxthreadcount >= 2);
		int length = std::distance(begin, end);
        if (length < SKIP_IF_LESS)
            cond_threads=false; // Don't use threads - too much overhead
        if (length == 1)
            return;
        int lnhalf = length/2;
        Iter split = begin;
        std::advance(split, lnhalf); // split up the list
        if (maxthreadcount < 0 || cond_threads)
        {
            unsigned new_mxthreadcount;
            if (cond_threads)
            {
                maxthreadcount -= 2;
                new_mxthreadcount = maxthreadcount / 2;
            }
            else
            {
                new_mxthreadcount = -1; // always use threads.
            }
            //std::bind(std::function<void(Iter, Iter, F, unsigned)>(p_sort_nthreads), begin, split, smaller, maxthreadcount/2) // did not work
            boost::shared_future<void> fu = boost::async([&begin, &split, smaller, maxthreadcount](){p_sort_nthreads(begin, split, smaller, maxthreadcount/2);});
            p_sort_nthreads(split, end, smaller, maxthreadcount/2);
            fu.get(); // also waits (like fu.wait())
            std::inplace_merge(begin, split, end, smaller);
        } // Using only one thread for maxthreadcount == 1 would be pointless and only produce time loss launching the thread
        else
        {
            std::sort(begin, end, smaller);
        }
	}

    // removed - error due to too many threads.
    /*template <typename Iter, typename F> void p_sort_allthreads(Iter begin, Iter end, F smaller) // uses threads down to the bottom "leaf" of the "tree" created by merge sort
    {
        p_sort_nthreads(begin, end, smaller, -1); // -1 means to always use threads
    }*/

    template <typename Iter, typename F> void p_sort_hwthreads(Iter begin, Iter end, F smaller, int SKIP_IF_LESS=SIL_DEF) // uses boost::thread::hardware_concurrency() threads, then uses std::sort
    {
        p_sort_nthreads(begin, end, smaller, boost::thread::hardware_concurrency(), SKIP_IF_LESS);
    }
}
