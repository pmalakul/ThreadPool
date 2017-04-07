#include <random>
#include <iomanip>
#include <numeric>
#include <chrono>
#include <vector>
#include <iostream>
#include <thread>

#ifdef MSVC
#include <concurrent_vector.h>
#include <ppl.h>
#endif

#include "getpi.h"
#include "getfib.h"




void doCalcs(size_t total_iterations, int & in_count_result) {
    auto seed = std::random_device{}();
    auto gen = std::mt19937{ seed };
    auto dist = std::uniform_real_distribution<>{0, 1};

    auto in_count{ 0 };

    //calculation
    for (auto counter = 0; counter < total_iterations; ++counter) {
        auto x = dist(gen);
        auto y = dist(gen);
        auto result = std::sqrt(std::pow(x, 2) + std::pow(y, 2));
        if (result < 1) {
            ++in_count; //check if the generated value is inside a unit circle
        }
    }

    in_count_result = in_count;
}

void getpi(size_t total_count) {

    std::chrono::time_point<std::chrono::system_clock> start, end;

    start = std::chrono::system_clock::now();

    int num_threads = std::thread::hardware_concurrency() - 1;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    std::vector<int> in_count;
    in_count.resize(num_threads);

    for (size_t i = 0; i < num_threads; ++i) {
        size_t total_iterations = total_count / num_threads;
        if (i == 0) {
            total_iterations += total_count % num_threads; // get the remaining iterations calculated by thread 0
        }

        // threads.emplace_back(doCalcs, total_iterations, std::ref(in_count[i]));
        threads.emplace_back( [total_iterations, i, &in_count] () {
            doCalcs(total_iterations, in_count[i]);
        });
    }

    for (auto & thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    double pi_value = 4.0 * static_cast<double>(std::accumulate(in_count.begin(), in_count.end(), 0)) 
        / static_cast<double>(total_count);
    // std::cout << "Value of PI is: " << pi_value << std::endl;
    std::cout << "Value of PI is: " << std::fixed << std::setprecision(9) << pi_value 
    << " at " << total_count << " iterations " << std::endl;

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Time used (multithread) : " << elapsed_seconds.count() << " sec" << std::endl;
}

void getpi_serial(size_t total_count) {

    std::chrono::time_point<std::chrono::system_clock> start, end;

    start = std::chrono::system_clock::now();

    int count = 0;

    doCalcs(total_count, count);

    double pi_value = 4.0 * static_cast<double>(count) / static_cast<double>(total_count);
    // std::cout << "Value of PI is: " << pi_value << std::endl;
    std::cout << "Value of PI is: " << std::fixed << std::setprecision(9) << pi_value 
    << " at " << total_count << " iterations " << std::endl;
    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Time used (serial) : " << elapsed_seconds.count() << " sec" << std::endl;
}


// Computes the nth Fibonacci number.
long long fibonacci(long long n) {
    if(n < 2) {
        return n;
    }
    return fibonacci(n-1) + fibonacci(n-2);
}

void getfibonacci() {
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();   

    // An array of Fibonacci numbers to compute.
    std::vector<int> a = { 41, 42, 43, 44, 45, 46, 47, 48};

    int num_threads = std::thread::hardware_concurrency() - 1;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    std::vector<unsigned long long> results;
    results.resize(a.size());

    size_t index = 0;
    for (size_t i = 0; i < num_threads && index < a.size() ; ++i) {
        size_t total_iterations = a.size() / num_threads;
        if (i == 0) {
            total_iterations += a.size() % num_threads; // get the remaining iterations calculated by thread 0
        }

        threads.emplace_back( [&a, &results, total_iterations, index] () {
            for (auto counter = 0; counter < total_iterations; ++counter) {
                results[index + counter] = fibonacci(a[index + counter]);
            }
        });
        // std::cout << i << ":" << index << ":" << total_iterations << std::endl;
        index += total_iterations;
        
    }

    for (auto & thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    for(unsigned long long i : results) {
        std::cout << i << std::endl;
    }

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Time used (multithread) : " << elapsed_seconds.count() << " sec" << std::endl;
}

void getfibonacci_serial() {
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();   

    // An array of Fibonacci numbers to compute.
    std::vector<int> a = { 41, 42, 43, 44, 45, 46, 47, 48 };

    std::vector<unsigned long long> results(a.size());
    for (size_t i = 0; i < a.size() ; ++i) {
        results[i] = fibonacci(a[i]);
        // std::cout << i << ":" << index << ":" << total_iterations << std::endl;
    }

    for(unsigned long long i : results) {
        std::cout << i << std::endl;
    }

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Time used (serial): " << elapsed_seconds.count() << " sec" << std::endl;
}

#ifdef MSVC
void getpi_ppl(size_t total_count, size_t num_tasks = std::thread::hardware_concurrency() - 1) {

    std::chrono::time_point<std::chrono::system_clock> start, end;

    start = std::chrono::system_clock::now();

    // size_t num_threads = std::thread::hardware_concurrency() - 1;
    // size_t num_threads = 1000;

    std::vector<int> in_count;
    in_count.resize(num_tasks);

    concurrency::parallel_for( (size_t) 0, num_tasks, [total_count, num_tasks, &in_count] (size_t i) {
        size_t total_iterations = total_count / num_tasks;
        if (i == 0) {
            total_iterations += total_count % num_tasks; // get the remaining iterations calculated by thread 0
        }
        doCalcs(total_iterations, in_count[i]);
    });

    double pi_value = 4.0 * static_cast<double>(std::accumulate(in_count.begin(), in_count.end(), 0)) 
        / static_cast<double>(total_count);
    // std::cout << "Value of PI is: " << pi_value << std::endl;
    std::cout << "Value of PI is: " << std::fixed << std::setprecision(9) << pi_value 
    << " at " << total_count << " iterations " << std::endl;

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Time used (ppl): " << elapsed_seconds.count() << " sec" << std::endl;
}

void getfibonacci_ppl() {
    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();   

    // An array of Fibonacci numbers to compute.
    std::vector<int> a = { 41, 42, 43, 44, 45, 46, 47, 48};

    // concurrency::concurrent_vector<unsigned long long> results;
    std::vector<unsigned long long> results;
    results.resize(a.size());

    concurrency::parallel_for( (size_t) 0, a.size(), [&] (size_t i) {
        results[i] = fibonacci(a[i]);
    });


    for(unsigned long long i : results) {
        std::cout << i << std::endl;
    }

    end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Time used (ppl): " << elapsed_seconds.count() << " sec" << std::endl;
}
#endif

int main() {

    // getpi_serial(1000000000);
    // getpi(1000000000);
    // getpi_ppl(1000000000);
    
    // getfibonacci_serial();
    // getfibonacci();
    // getfibonacci_ppl();
    
    std::function<void (std::string const &)> func = 
      [] (std::string const & result) {
        std::cout << result << std::endl;
    };

    PMConcurrency::ThreadPool threadPool;

    // std::shared_ptr<TP::getpi> myGetPi = std::make_shared<TP::getpi>(threadPool, threadPool.get_thread_size(), func);
    // myGetPi->start();

    std::shared_ptr<TP::getfib> myGetFib = std::make_shared<TP::getfib>(threadPool, threadPool.get_thread_size(), func);
    myGetFib->start();

    threadPool.startMainIoService();
    return 0;
}