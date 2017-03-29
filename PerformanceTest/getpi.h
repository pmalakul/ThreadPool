#ifndef _GETPI_H
#define _GETPI_H

#include "ThreadPool.h"

namespace TP {
    class getpi : public std::enable_shared_from_this<getpi> {
    public:
        getpi(PMConcurrency::ThreadPool & threadPool, size_t num_tasks,
            std::function<void (std::string const &)> myFunc);

        ~getpi();

        void start();

    private:
        void getWorkLoad(size_t & workload, size_t & remainload, size_t & iter,
            size_t min, size_t count, size_t num_tasks);
        void doCalcs(size_t total_iterations, int & in_count_result);
        void run(size_t total_count, size_t minload);
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

        // // PI
        
        std::vector<int> _in_count;

        size_t _num_tasks;

    };    
}



#endif