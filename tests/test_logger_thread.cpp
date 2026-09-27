/**
 * @file test_logger_thread.cpp
 * @brief Thread-safety and producer-consumer regression test for LoggerThread.
 *
 * Implemented using pure standard C++ (no Qt dependencies) to allow clean execution
 * under ThreadSanitizer (TSAN) without false positives from Qt internals.
 */

#include "LoggerThread.h"

#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <string>

int main() {
    std::cout << "[TEST] Starting LoggerThread pure C++ multithreading test...\n";

    const std::string testLogPath = "logs/test_concurrency.log";
    const std::string rotatedLogPath = testLogPath + ".1";

    // Clean up any stale test logs
    std::filesystem::remove(testLogPath);
    std::filesystem::remove(rotatedLogPath);

    constexpr int NUM_PRODUCERS = 4;
    constexpr int MSGS_PER_PRODUCER = 1000;
    constexpr int TOTAL_EXPECTED = NUM_PRODUCERS * MSGS_PER_PRODUCER;

    {
        LoggerThread logger(testLogPath, 10 * 1024 * 1024); // 10MB limit (no rotation during this phase)
        logger.start();
        assert(logger.isRunning());

        std::vector<std::thread> producers;
        producers.reserve(NUM_PRODUCERS);

        for (int p = 0; p < NUM_PRODUCERS; ++p) {
            producers.emplace_back([&logger, p]() {
                for (int i = 0; i < MSGS_PER_PRODUCER; ++i) {
                    logger.push("Producer " + std::to_string(p) + " msg #" + std::to_string(i));
                }
            });
        }

        // Wait for all producer threads to finish pushing
        for (auto &t : producers) {
            t.join();
        }

        // Graceful stop drains remaining queued entries and joins consumer thread
        logger.stop();
        assert(!logger.isRunning());

        std::cout << "[TEST] Processed: " << logger.getProcessedCount()
                  << ", Dropped: " << logger.getDroppedCount() << "\n";

        assert(logger.getProcessedCount() == static_cast<size_t>(TOTAL_EXPECTED));
        assert(logger.getDroppedCount() == 0);
    }

    // Verify line count on disk
    {
        std::ifstream file(testLogPath);
        assert(file.is_open());
        size_t lines = 0;
        std::string line;
        while (std::getline(file, line)) {
            lines++;
        }
        file.close();

        std::cout << "[TEST] Written lines on disk: " << lines << " (expected: " << TOTAL_EXPECTED << ")\n";
        assert(lines == static_cast<size_t>(TOTAL_EXPECTED));
    }

    // Test log rotation functionality
    std::cout << "[TEST] Verifying continuous size-based log rotation...\n";
    {
        const std::string rotateTestPath = "logs/test_rotation.log";
        const std::string rotateBackupPath = rotateTestPath + ".1";
        std::filesystem::remove(rotateTestPath);
        std::filesystem::remove(rotateBackupPath);

        // Set low rotation threshold: 2 KB
        LoggerThread rotLogger(rotateTestPath, 2048);
        rotLogger.start();

        // Push enough payload to trigger rotation multiple times
        for (int i = 0; i < 200; ++i) {
            rotLogger.push("Sample log line for continuous rotation test padding string sequence #" + std::to_string(i));
        }

        rotLogger.stop();

        assert(std::filesystem::exists(rotateTestPath));
        assert(std::filesystem::exists(rotateBackupPath));
        assert(std::filesystem::file_size(rotateBackupPath) > 0);
        std::cout << "[TEST] Rotation verified. Backup log size: "
                  << std::filesystem::file_size(rotateBackupPath) << " bytes.\n";

        std::filesystem::remove(rotateTestPath);
        std::filesystem::remove(rotateBackupPath);
    }

    // Clean up test files
    std::filesystem::remove(testLogPath);
    std::filesystem::remove(rotatedLogPath);

    std::cout << "[PASS] LoggerThread producer-consumer concurrency and rotation test passed successfully!\n";
    return 0;
}
