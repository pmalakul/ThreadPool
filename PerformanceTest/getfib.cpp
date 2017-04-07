#include "getfib.h"
#include <random>
#include <iomanip>
#include <numeric>

#define LOGGER_ENABLED
#include "logger.h"

using namespace TP;

getfib::getfib(PMConcurrency::ThreadPool & threadPool, size_t num_tasks, std::function<void (std::string const &)> myFunc)
  : _threadPool(threadPool), _num_tasks(num_tasks), _func(myFunc) {

  _threadPool.start();
  _a = { 41, 42, 43, 44, 45, 46, 47, 48 };
}

getfib::~getfib() {
    LOG("getfib destroyed..");
}


void getfib::start() {

    run();
    // _threadPool.startMainIoService();

}



long long getfib::fibonacci(long long n) {
    if(n < 2) {
        return n;
    }
    return fibonacci(n-1) + fibonacci(n-2);
}

void getfib::getWorkLoad(size_t & workload, size_t & remainload, size_t & iter, 
  size_t min, size_t count, size_t num_tasks ) {

  size_t load = count / num_tasks;
  size_t remain = count % num_tasks;

  if ( load < min) {
    if (count < min) {
      workload = count;
      iter = 1;
      remainload = workload;
    }
    else {
      workload = min;
      iter = count / workload;
      remainload = count % workload;
      if(remainload == 0) {
        remainload = workload;
      }
      else {
        iter++;
      }
    }
  }
  else { // load >= MIN
    workload = load;
    iter = count / workload;
    remainload = count % workload;
    if(remainload == 0) {
      remainload = workload;
    }
    else {
      iter++;
    }
  }
}

void getfib::run() {

  // std::cout << "Using ASIO threadpool for fibonacci." << std::endl;
  startTime();
  // An array of Fibonacci numbers to compute.
  size_t workload, remainload, iter;
  getWorkLoad(workload, remainload, iter, 1, _a.size(), _num_tasks);
  LOG("workload: %d, remainlod: %d, iter: %d", workload, remainload, iter);

  _results.resize(_a.size());

  size_t index = 0;
  for(size_t i = 0; i < iter; i++) {
    // LOG("%d", index);
    if (i < iter - 1) {
      auto self(shared_from_this());
      addWork( [this, self, workload, index] () {
          // std::cout << "Start tid: " << _threadPool.getThisThreadId() << std::endl;
          for (size_t counter = 0; counter < workload; ++counter) {
              _results[index + counter] = fibonacci(_a[index + counter]);
          }
          auto self(shared_from_this());
          _threadPool.strand( [this, self] () {
              joinWorks();
          });
      });
      index += workload;
    }
    else {
      auto self(shared_from_this());
      addWork( [this, self, remainload, index] () {
          // std::cout << "Start tid: " << _threadPool.getThisThreadId() << std::endl;
          for (size_t counter = 0; counter < remainload; ++counter) {
              _results[index + counter] = fibonacci(_a[index + counter]);
          }
          auto self(shared_from_this());
          _threadPool.strand( [this, self] () {
              joinWorks();
          });
      });
      index += remainload;
    }
  }



  // size_t num_threads = _threadPool.get_thread_size();

  // _results.resize(_a.size());

  // size_t index = 0;
  // for (size_t i = 0; i < num_threads && index < _a.size() ; ++i) {
  //     size_t total_iterations = _a.size() / num_threads;
  //     if (i == 0) {
  //         total_iterations += _a.size() % num_threads; // get the remaining iterations calculated by thread 0
  //     }
  //     auto self(shared_from_this());
  //     addWork( [this, self, total_iterations, index] () {
  //         for (auto counter = 0; counter < total_iterations; ++counter) {
  //             _results[index + counter] = fibonacci(_a[index + counter]);
  //         }
  //         auto self(shared_from_this());
  //         _threadPool.strand( [this, self] () {
  //             joinWorks();
  //         });
  //     });
  //     // std::cout << i << ":" << index << ":" << total_iterations << std::endl;
  //     index += total_iterations;
      
  // }
}

void getfib::joinWorks() {
  --_works;
  if (_works == 0 ) {
    auto self(shared_from_this());
    _threadPool.enqueue( [this, self] () {
      for(unsigned long long i : _results) {
          std::cout << i << std::endl;
      }
      auto self(shared_from_this());
      _threadPool.enqueue([this, self] () {
        endTime();
        auto self(shared_from_this());
        _threadPool.enqueueMainIoService( [this, self] () {
          _threadPool.stopMainIoService();
        });
      });
    });
  }

}

void getfib::startTime() {
  _start = std::chrono::system_clock::now();

}

void getfib::endTime() {
  _end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = _end - _start;
  std::cout << "Time used (asio) : " << elapsed_seconds.count() << " sec" << std::endl;


}


void getfib::runNativePi(size_t total_count) {
    // std::cout << "Using native multithread with iteration = " << total_count << std::endl;
    // startTime();

    // int num_threads = std::thread::hardware_concurrency() - 1;

    // std::vector<std::thread> threads;
    // threads.reserve(num_threads);
  
    // std::vector<int> in_count(num_threads);

    // for (size_t i = 0; i < num_threads; ++i) {
    //     size_t total_iterations = total_count / num_threads;
    //     if (i == 0) {
    //         total_iterations += total_count % num_threads; // get the remaining iterations calculated by thread 0
    //     }

    //     threads.emplace_back( [this, total_iterations, i, &in_count] () {
    //         doCalcs(total_iterations, in_count[i]);
    //     });
    // }

    // for (auto & thread : threads) {
    //     if (thread.joinable()) {
    //         thread.join();
    //     }
    // }

    // double pi_value = 4.0 * static_cast<double>(std::accumulate(in_count.begin(), in_count.end(), 0)) 
    //     / static_cast<double>(total_count);
    // // std::cout << "Value of PI is: " << pi_value << std::endl;
    // std::cout << "Value of PI is: " << std::fixed << std::setprecision(9) << pi_value << std::endl;

    // endTime();
}