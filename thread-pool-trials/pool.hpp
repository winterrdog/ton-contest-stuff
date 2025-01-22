#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace my_thread_pool {
// struct to represent a task queue for a worker thread
struct Queue {
  std::deque<std::function<void()>> tasks_dq; // deque to store tasks
  std::mutex q_mutex; // mutex to control access to the task queue
};

// thread pool class with a fixed number of threads and capabale of working with
// multiple queues and stealing tasks
class ThreadPool {
public:
  explicit ThreadPool(size_t thread_cnt);
  ~ThreadPool();

  // adds task to worker pool's task queue
  template <class F, class... Args>
  [[nodiscard("please make sure the output is used by producer")]] auto
  enqueue(F &&fn, Args &&...args)
      -> std::future<typename std::invoke_result<F, Args...>::type> {
    /*
    algorithm:
        1. create a shared pointer to a packaged_task
        2. wrap the task in a lambda
        3. push the lambda to the task queue
        4. notify all threads to check for new tasks
    */

    using ret_type = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<ret_type()>>(
        std::bind(std::forward<F>(fn),
                  std::forward<Args>(args)...) // bind args to function
    );

    std::future<ret_type> res(task->get_future());
    size_t worker_id = next_task_id_++ % workers_task_queues_.size();
    {
      std::unique_lock<std::mutex> lock(
          workers_task_queues_[worker_id]->q_mutex);
      if (stop_.load()) {
        throw std::runtime_error(
            "cannot send in a task on a stopped thread pool.");
      }

      workers_task_queues_[worker_id]->tasks_dq.emplace_back(
          [task]() { return (*task)(); });
    }

    condition_.notify_all(); // notify all threads to check for new tasks
    return res;
  }

private:
  std::vector<std::thread> workers_; // vector of worker threads

  // vector of task queues for each worker thread
  std::vector<std::unique_ptr<Queue>> workers_task_queues_;

  std::mutex pool_mutex_; // control safe access to task queue

  // condition variable to notify threads of new tasks
  std::condition_variable condition_;
  std::atomic<bool> stop_; // flag to stop the thread pool

  // id of the next task to be added to the pool
  std::atomic<size_t> next_task_id_{0};

  void worker_thread(size_t worker_id) {
    for (;;) {
      std::function<void()> task;

      // get a task, if u dont hv one then steal one, otherwise wait for some to
      // come by
      if (try_get_task(worker_id, task) || try_steal_task(worker_id, task)) {
        execute_task(task);
      } else {
        // Wait for tasks or stop signal
        std::unique_lock<std::mutex> lock(pool_mutex_);
        condition_.wait(lock, [this, worker_id, &task]() {
          return stop_.load() ||
                 !workers_task_queues_[worker_id]->tasks_dq.empty() ||
                 try_steal_task(worker_id, task);
        });

        if (stop_.load())
          return;

        if (task) {
          execute_task(task);
        }
      }
    }
  }

  bool try_get_task(size_t worker_id, std::function<void()> &task) {
    /*
    algorithm:
        1. lock the task queue
        2. check if the task queue is empty
        3. if not empty, get the task off the dequeue belonging to the worker
    */

    auto &curr_task_queue = workers_task_queues_[worker_id];

    std::lock_guard<std::mutex> lock(curr_task_queue->q_mutex);
    if (curr_task_queue->tasks_dq.empty()) {
      return false; // no work for thread
    }

    // get the task off the dequeue
    task = std::move(curr_task_queue->tasks_dq.front());
    curr_task_queue->tasks_dq.pop_front();

    return true;
  }

  bool try_steal_task(size_t worker_id, std::function<void()> &task) {
    /*
    algorithm:
        1. for each worker thread, except the current one
        2. lock the task queue
        3. check if the task queue is empty
        4. if not empty, steal the task from the 'back' of the dequeue belonging
    to the worker with work available
    */

    for (size_t i = 0; i < workers_task_queues_.size(); ++i) {
      if (i != worker_id) {
        auto &victim_queue = workers_task_queues_[i];
        std::lock_guard<std::mutex> lock(victim_queue->q_mutex);

        // steal the task from those with work available, provided they hv
        if (!victim_queue->tasks_dq.empty()) {
          task = std::move(victim_queue->tasks_dq.back());
          victim_queue->tasks_dq.pop_back();

          return true;
        }
      }
    }
    return false;
  }

  void execute_task(std::function<void()> &task) {
    try {
      task();
    } catch (const std::exception &e) {
      std::cerr << "task threw exception: " << e.what() << '\n';
    } catch (...) {
      std::cerr << "task threw an unknown exception" << '\n';
    }
  }
};
} // namespace my_thread_pool
