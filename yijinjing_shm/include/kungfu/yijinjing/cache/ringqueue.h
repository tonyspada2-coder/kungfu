
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <cstddef>
#include <kungfu/common.h>
#include <mutex>

namespace kungfu::yijinjing::cache {
template <typename T> class ringqueue {
public:
  explicit ringqueue(size_t capacity) {
    capacityMask_ = capacity - 1;
    for (size_t i = 1; i <= sizeof(void *) * 4; i <<= 1) {
      capacityMask_ |= capacityMask_ >> i;
    }
    capacity_ = capacityMask_ + 1;
    queue_ = (T *)new char[sizeof(T) * capacity_];
    pop_value_ = (T *)new char[sizeof(T)];
    tail_.store(0, std::memory_order_relaxed);
    head_.store(0, std::memory_order_relaxed);
  }

  ringqueue(ringqueue &&that)
      : capacity_(that.capacity_), capacityMask_(that.capacityMask_), head_(that.head_.load(std::memory_order_relaxed)),
        tail_(that.tail_.load(std::memory_order_relaxed)), queue_(that.queue_), pop_value_(that.pop_value_) {
    that.queue_ = nullptr;
    that.pop_value_ = nullptr;
  }

  ringqueue(const ringqueue &that)
      : capacity_(that.capacity_), capacityMask_(that.capacityMask_), head_(that.head_.load(std::memory_order_relaxed)),
        tail_(that.tail_.load(std::memory_order_relaxed)), queue_((T *)new char[sizeof(T) * that.capacity_]) {
    for (int i = 0; i < that.size(); i++) {
      T *node = queue_ + i;
      new (node) T(*(that.queue_ + i));
    }
    new (pop_value_) T(*(that.pop_value_));
  }

  ~ringqueue() {
    for (size_t i = head_.load(std::memory_order_relaxed); i != tail_.load(std::memory_order_relaxed); ++i)
      (&queue_[i & capacityMask_])->~T();
    (pop_value_)->~T();
    delete[] reinterpret_cast<char *>(queue_);
    delete[] reinterpret_cast<char *>(pop_value_);
  }
  size_t capacity() const { return capacity_; }
  size_t size() const {
    size_t head = head_.load(std::memory_order_acquire);
    return tail_.load(std::memory_order_relaxed) - head;
  }

  bool push(const T &p_data) {
    if (size() >= capacity_) {
      return false;
    }
    T *node;
    size_t tail = tail_.fetch_add(1, std::memory_order_relaxed);
    node = &queue_[tail & capacityMask_];
    memset(node, 0, sizeof(T));
    new (node) T(p_data);
    return true;
  }

  bool pop(T *&result) {
    T *node;
    result = nullptr;
    size_t head = head_.load(std::memory_order_relaxed);
    size_t tail = tail_.load(std::memory_order_acquire);
    if (head >= tail) {
      return false;
    }
    node = &queue_[head & capacityMask_];
    memset(pop_value_, 0, sizeof(T));
    *pop_value_ = *node;
    result = pop_value_;
    node->~T();
    head_.fetch_add(1, std::memory_order_relaxed);
    return true;
  }

private:
  size_t capacityMask_;
  T *queue_;
  T *pop_value_;
  size_t capacity_;
  std::atomic<size_t> tail_;
  std::atomic<size_t> head_;
};
} // namespace kungfu::yijinjing::cache
