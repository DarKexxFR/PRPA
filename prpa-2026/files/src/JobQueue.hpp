#pragma once

#include "Image.hpp"
#include <thread>
#include <optional>
#include <mutex>

class JobQueue
{
public:
  static constexpr int MAX_CAPACITY = 100;

  using query_t  = Image<rgb8>;

  JobQueue(int capacity);

  // Add a job in the queue. If the maximum capacity is reached
  // the oldest job is removed
  void enqueue(Image<rgb8>) noexcept;

  // Pop the oldest job from the queue (nullopt if fempty)
  std::optional<query_t> dequeue() noexcept;


  bool empty() const noexcept;

private:
  mutable std::mutex m_fat_lock;
  query_t   m_ring[MAX_CAPACITY];
  int       m_capacity;
  int       m_pos  = 0; // Start position
  int       m_size = 0;
};
