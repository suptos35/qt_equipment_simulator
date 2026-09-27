/**
 * @file LoggerThread.h
 * @brief High-throughput, thread-safe asynchronous disk logger.
 *
 * Implements a producer-consumer architecture using standard C++ synchronization
 * primitives (std::mutex, std::condition_variable, std::thread) without Qt dependencies.
 * Designed to ensure non-blocking logging for hard-real-time loops and safe execution
 * under ThreadSanitizer (TSAN).
 */

#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <fstream>
#include <cstddef>

/**
 * @class LoggerThread
 * @brief Background disk logging worker with bounded queue and continuous size rotation.
 */
class LoggerThread {
public:
    static constexpr size_t DEFAULT_MAX_FILE_SIZE = 5 * 1024 * 1024; // 5 MB
    static constexpr size_t MAX_QUEUE_CAPACITY = 20000;

    /**
     * @brief Constructs LoggerThread targeting a file path.
     * @param logFilePath Path to log file.
     * @param maxFileSize Threshold in bytes for log rotation.
     */
    explicit LoggerThread(std::string logFilePath = "logs/equipment.log",
                         size_t maxFileSize = DEFAULT_MAX_FILE_SIZE);

    /**
     * @brief Destructs LoggerThread, ensuring graceful drain and worker join.
     */
    ~LoggerThread();

    // Non-copyable, non-movable
    LoggerThread(const LoggerThread &) = delete;
    LoggerThread &operator=(const LoggerThread &) = delete;
    LoggerThread(LoggerThread &&) = delete;
    LoggerThread &operator=(LoggerThread &&) = delete;

    /**
     * @brief Starts the background consumer thread.
     */
    void start();

    /**
     * @brief Stops the background consumer thread gracefully after draining queue.
     */
    void stop();

    /**
     * @brief Pushes a formatted log entry into the synchronized queue.
     *
     * The critical section strictly guards queue insertion; no disk I/O occurs
     * while the lock is acquired.
     * @param message Log message line.
     */
    void push(std::string message);

    /**
     * @brief Returns total number of log entries written to disk.
     */
    [[nodiscard]] size_t getProcessedCount() const noexcept;

    /**
     * @brief Returns total number of log entries dropped due to overflow.
     */
    [[nodiscard]] size_t getDroppedCount() const noexcept;

    /**
     * @brief Returns current queue size.
     */
    [[nodiscard]] size_t getQueueSize() const;

    /**
     * @brief Checks if background consumer thread is active.
     */
    [[nodiscard]] bool isRunning() const noexcept;

    /**
     * @brief Flushes pending entries and ensures disk write.
     */
    void flush();

    /**
     * @brief Global singleton instance for system-wide message routing.
     */
    static LoggerThread &instance();

private:
    void runWorker();
    void checkAndRotate();

    std::string m_logFilePath;
    size_t m_maxFileSize;

    std::queue<std::string> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_workerThread;

    std::atomic<bool> m_running{false};
    std::atomic<size_t> m_processedCount{0};
    std::atomic<size_t> m_droppedCount{0};

    std::ofstream m_fileStream;
};
