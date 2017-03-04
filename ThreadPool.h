// MIT License

// Copyright (c) 2017 Poom Malakul Na Ayudhya

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <asio.hpp>
#include <thread>
#include <iostream>
#include <sstream>

namespace PMConcurrency {
	
	class MainIoService {
	public:
		MainIoService() {}

		~MainIoService() {
			_work.reset();
		}

		template<typename T> // T must be "void handler()""
		void enqueue(T f){
			_io_service.post(f);
		}

		void start() {
			_work.reset(new asio::io_service::work(_io_service));
			_io_service.run();
		}

		void stop() {
			if(_work) {
				_work.reset();
			}
		}

	private:
		asio::io_service _io_service;
		std::unique_ptr<asio::io_service::work> _work;
	};


	class ThreadPool {
	public:
		ThreadPool(size_t threads = std::thread::hardware_concurrency() - 1) 
			:  _thread_size(threads), _strand(_io_service) {
		}

		~ThreadPool() {
			_work.reset(); //stop all, allow run() to exit
			
			for (auto& thread : _group) {
      			if (thread.joinable()) {
      				thread.join();
      			}
  			}
		}

		template<typename T> // T must be "void handler()""
		void enqueue(T f) {
			_io_service.post(f);
		}
		
		template<typename T> // T must be "void handler()""
		void strand(T f) {
			_io_service.post(_strand.wrap(f));
		}

		asio::io_service & get_io_service() {
			return _io_service;
		}
		
		size_t get_thread_size() {
			return _thread_size;
		}

		void start() {
			_work.reset(new asio::io_service::work(_io_service));
			for ( std::size_t i = 0; i < _thread_size; ++i ) {
				_group.emplace_back( [this] () {
					try {
						_io_service.run();
					}
					catch(...) {
						_eptr = std::current_exception();
						_main_io_service.enqueue([this] () {
							std::rethrow_exception(_eptr);
						});
					}
				});
			}			
		}

		void stop() {
			if(_work) {
				_work.reset();	
			}
			
			for (auto& thread : _group) {
				if (thread.joinable()) {
					thread.join();
				}
			}
			if(_io_service.stopped()) {
				_io_service.reset();
			}
			_group.clear();
		}

		template<typename T>
		void enqueueMainIoService(T f) {
			_main_io_service.enqueue(f);
		}

		void startMainIoService() {
			_main_io_service.start();
		}

		void stopMainIoService() {
			_main_io_service.stop();
		}

		std::string getThisThreadId() {
    		std::ostringstream ss;
    		ss << std::this_thread::get_id();
    		return ss.str();
		}

		void checkError() {
			if(_eptr) {
				std::rethrow_exception(_eptr);
			}
		}


	private:
		
		std::exception_ptr _eptr;
		size_t _thread_size;
		MainIoService _main_io_service;
		asio::io_service _io_service; //< the io_service we are wrapping
		std::unique_ptr<asio::io_service::work> _work;
		asio::io_service::strand _strand;
		std::vector<std::thread> _group;  //< need to keep track of threads so we can join them
	};



}

#endif 