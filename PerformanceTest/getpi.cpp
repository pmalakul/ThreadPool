#include "getpi.h"
#include <random>
#include <iomanip>
#include <numeric>

#define LOGGER_ENABLED
#include "logger.h"

using namespace TP;

getpi::getpi(PMConcurrency::ThreadPool & threadPool, size_t num_tasks, 
  std::function<void (std::string const &)> myFunc)
  : _threadPool(threadPool), _num_tasks(num_tasks),  _func(myFunc) {

  _threadPool.start();
}

getpi::~getpi() {
    LOG("getpi destroyed..");
}


void getpi::start() {

    run(1000000000, 1000);
    // _threadPool.startMainIoService();

}



void getpi::doCalcs(size_t total_iterations, int & in_count_result) {
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

// void getpi::getWorkLoad(size_t min, size_t count, size_t threadSize, 
//   size_t & workload, size_t & remainload, size_t & iter) {

//   size_t load = count / threadSize;
//   size_t remain = count % threadSize;

//   if ( load < min) {
//     if(load == 0) {
//       workload = remain;
//       remainload = 0;
//       iter = 1;
//     }
//     else {
//       if( count < min) {
//         workload = count;
//         remainload = 0;
//         iter = 1;
//       }
//       else {        // count >= MIN
//         workload = min;
//         iter = count / workload;
//         remainload = count % workload;
//         if(remainload > 0) {
//           iter++;
//         }
//       }
//     } 
//   }
//   else { // load >= MIN
//     workload = load;
//     iter = count / workload;
//     remainload = count % workload;
//     if(remainload > 0) {
//       iter++;
//     }
//   }

// }

void getpi::getWorkLoad(size_t & workload, size_t & remainload, size_t & iter,
  size_t min, size_t count, size_t num_tasks) {

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

void getpi::run(size_t total_count, size_t minload) {
  // std::cout << "Using ASIO threadpool with iteration = " << total_count << std::endl;
  startTime();

  _total_count = total_count;

  size_t workload, remainload, iter;
  getWorkLoad( workload, remainload, iter, minload, total_count, _num_tasks);
  // getWorkLoad( workload, remainload, iter, minload, total_count, 1000);
  LOG("workload: %d, remainlod: %d, iter: %d", workload, remainload, iter);
  
  _in_count.resize(iter);

  for(size_t i = 0; i < iter; i++) {
    if (i < iter - 1) {
      auto self(shared_from_this());
      addWork( [this, self, workload, i] () {
          // std::cout << "Start tid: " << _threadPool.getThisThreadId() << std::endl;
          doCalcs( workload, _in_count[i]);
          auto self(shared_from_this());
          _threadPool.strand( [this, self] () {
              joinWorks();
          });
      });
    }
    else {
      auto self(shared_from_this());
      addWork( [this, self, remainload, i] () {
          // std::cout << "Start tid: " << _threadPool.getThisThreadId() << std::endl;
          doCalcs( remainload, _in_count[i]);
          auto self(shared_from_this());
          _threadPool.strand( [this, self] () {
              joinWorks();
          });
      });
    }
  }

}

void getpi::joinWorks() {
  --_works;
  if (_works == 0 ) {
    auto self(shared_from_this());
    _threadPool.enqueue(
      [this, self] () {
        double pi_value = 4.0 * static_cast<double>(std::accumulate(_in_count.begin(), _in_count.end(), 0)) 
              / static_cast<double>(_total_count);
        std::cout << "Value of PI is: " << std::fixed << std::setprecision(9) << pi_value 
        << " at " << _total_count << " iterations " << std::endl;
        auto self(shared_from_this());
        _threadPool.enqueue(
          [this, self] () {
            endTime();
            // throw std::runtime_error("getpi throw exception");
            auto self(shared_from_this());
            _threadPool.enqueueMainIoService(
              [this, self] () {
                _threadPool.stopMainIoService();
            });

        });

    });
  }

}

void getpi::startTime() {
  _start = std::chrono::system_clock::now();

}

void getpi::endTime() {
  _end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = _end - _start;
  std::cout << "Time used (asio) : " << elapsed_seconds.count() << " sec" << std::endl;


}


void getpi::runNativePi(size_t total_count) {
    std::cout << "Using native multithread with iteration = " << total_count << std::endl;
    startTime();

    int num_threads = std::thread::hardware_concurrency() - 1;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    std::vector<int> in_count(num_threads);

    for (size_t i = 0; i < num_threads; ++i) {
        size_t total_iterations = total_count / num_threads;
        if (i == 0) {
            total_iterations += total_count % num_threads; // get the remaining iterations calculated by thread 0
        }

        threads.emplace_back( [this, total_iterations, i, &in_count] () {
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
    std::cout << "Value of PI is: " << std::fixed << std::setprecision(9) << pi_value << std::endl;

    endTime();
}