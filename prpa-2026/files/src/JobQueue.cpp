#include "JobQueue.hpp"

JobQueue::JobQueue(int capacity)
    : m_capacity {capacity}
{
}

void JobQueue::enqueue(Image<rgb8> x) noexcept
{
  std::lock_guard<std::mutex> l(m_fat_lock);
  if (m_size == m_capacity)
  {
    std::swap(m_ring[m_pos], x);
    m_pos = (m_pos + 1) % m_capacity;
  }
  else
  {
    int i     = (m_pos + m_size++) % m_capacity;
    m_ring[i] = std::move(x);
  }
}

std::optional<JobQueue::query_t> JobQueue::dequeue() noexcept
{
  std::lock_guard<std::mutex> l(m_fat_lock);

  if (m_size == 0)
    return std::nullopt;

  auto q = std::move(m_ring[m_pos]);
  m_pos  = (m_pos + 1) % m_capacity;
  m_size--;
  return q;
}

bool JobQueue::empty() const noexcept
{
  std::lock_guard<std::mutex> l(m_fat_lock);
  return m_size == 0;
}
