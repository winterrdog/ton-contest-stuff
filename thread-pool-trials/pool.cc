#include "pool.hpp"

using my_thread_pool::ThreadPool;

// constructor
ThreadPool::ThreadPool(size_t num_workers) : stop_{false} {
  for (size_t i = 0; i != num_workers; i++) {
    workers_task_queues_.emplace_back(std::make_unique<Queue>());
    workers_.emplace_back([this, i]() { worker_thread(i); });
  }
}

ThreadPool::~ThreadPool() {
  /*
    algorithm:
        1. set the stop flag to true
        2. notify all worker threads that the pool has been stopped
        3. join all worker threads( makes sure that all worker threads have
    finished executing )
    */
  stop_.store(true);
  condition_.notify_all();

  for (size_t i = 0; i != workers_.size(); i++) {
    if (workers_[i].joinable())
      workers_.at(i).join();
  }
}
