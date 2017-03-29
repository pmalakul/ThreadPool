#ifndef _GETFIB_H
#define _GETFIB_H

#include "ThreadPool.h"

namespace TP {
    class getfib : public std::enable_shared_from_this<getfib> {
    public:
        getfib(PMConcurrency::ThreadPool & threadPool, size_t num_tasks,
            std::function<void (std::string const &)> myFunc);

        ~getfib();

        void start();

    private:
        void getWorkLoad(size_t & workload, size_t & remainload, size_t & iter,
            size_t min, size_t count, size_t num_tasks);
        long long fibonacci(long long n);

        void run();
        void runNativePi(size_t total_count);

        size_t _total_count;

        template<typename T> // T must be "void handler()""
        void addWork(T f){
          _threadPool.enqueue(f);
          ++_works;
        }

        void joinWorks();
        int _works = 0;

        void startTime();
        void endTime();
        std::chrono::time_point<std::chrono::system_clock> _start, _end;


        PMConcurrency::ThreadPool & _threadPool;

        std::function<void (std::string const &)> _func;

        // // FIBONACCI
        std::vector<int> _a;
        std::vector<unsigned long long> _results;

        size_t _num_tasks;

    };    
}



#endif