#pragma once

#include <cstdint>
#include <vector>
#include <thread>
#include <functional>
#include <optional>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>

#include <tracy/Tracy.hpp>

namespace vx3d
{
    class thread_pool
    {
    public:
        explicit thread_pool(std::uint32_t thread_count);

        ~thread_pool();

        void submit_task(const std::function<void()> &task);

        void submit_tasks(const std::vector<std::function<void()>> &tasks);

        void flush();

    private:
        [[nodiscard]] std::optional<std::function<void()>> _next_task();

        void _thread_task();

        std::mutex _work_lock;

        std::condition_variable _work_conditional;
        std::condition_variable _job_finished_conditional;

        std::queue<std::function<void()>> _tasks;
        std::vector<std::thread>          _threads;
    };
}
