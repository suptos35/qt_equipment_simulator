/**
 * @file LoggerThread.cpp
 * @brief Implementation of high-throughput asynchronous file logging worker.
 */

#include "LoggerThread.h"

#include <filesystem>
#include <iostream>
#include <utility>

LoggerThread::LoggerThread(std::string logFilePath, size_t maxFileSize)
    : m_logFilePath(std::move(logFilePath)),
      m_maxFileSize(maxFileSize) {
}

LoggerThread::~LoggerThread() {
    stop();
}

LoggerThread &LoggerThread::instance() {
    static LoggerThread s_instance("logs/equipment.log");
    return s_instance;
}

void LoggerThread::start() {
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true)) {
        return; // Already running
    }

    // Ensure target log directory exists
    try {
        std::filesystem::path p(m_logFilePath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (...) {
        // Fallback for directory creation issues
    }

    m_fileStream.open(m_logFilePath, std::ios::out | std::ios::app);
    m_workerThread = std::thread(&LoggerThread::runWorker, this);
}

void LoggerThread::stop() {
    bool expected = true;
    if (!m_running.compare_exchange_strong(expected, false)) {
        return; // Already stopped or stopping
    }

    // Wake up worker thread so it can drain remaining queue and exit
    m_cv.notify_all();

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    if (m_fileStream.is_open()) {
        m_fileStream.flush();
        m_fileStream.close();
    }
}

void LoggerThread::push(std::string message) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.size() < MAX_QUEUE_CAPACITY) {
            m_queue.push(std::move(message));
        } else {
            m_droppedCount++;
            return;
        }
    }
    // Signal consumer outside the critical section to reduce lock contention
    m_cv.notify_one();
}

void LoggerThread::runWorker() {
    while (true) {
        std::string entry;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() {
                return !m_queue.empty() || !m_running.load(std::memory_order_relaxed);
            });

            if (!m_running.load(std::memory_order_relaxed) && m_queue.empty()) {
                break;
            }

            if (!m_queue.empty()) {
                entry = std::move(m_queue.front());
                m_queue.pop();
            }
        }

        // CRITICAL: Disk I/O performed strictly outside the lock
        if (!entry.empty()) {
            if (m_fileStream.is_open()) {
                m_fileStream << entry << "\n";
                m_fileStream.flush();
                m_processedCount++;
                checkAndRotate();
            }
        }
    }
}

void LoggerThread::checkAndRotate() {
    try {
        if (!std::filesystem::exists(m_logFilePath)) {
            return;
        }

        std::uintmax_t currentSize = std::filesystem::file_size(m_logFilePath);
        if (currentSize >= m_maxFileSize) {
            m_fileStream.close();

            std::string backupPath = m_logFilePath + ".1";
            std::error_code ec;
            std::filesystem::rename(m_logFilePath, backupPath, ec);

            m_fileStream.open(m_logFilePath, std::ios::out | std::ios::trunc);
        }
    } catch (...) {
        // In mission-critical logging, rotation errors must not crash the equipment
    }
}

size_t LoggerThread::getProcessedCount() const noexcept {
    return m_processedCount.load();
}

size_t LoggerThread::getDroppedCount() const noexcept {
    return m_droppedCount.load();
}

size_t LoggerThread::getQueueSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

bool LoggerThread::isRunning() const noexcept {
    return m_running.load();
}

void LoggerThread::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.flush();
    }
}
