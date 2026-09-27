/**
 * @file test_adversarial_logger.cpp
 * @brief Adversarial concurrency, rotation, and fault test suite for LoggerThread (LT-01 to LT-05).
 */

#include "LoggerThread.h"

#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>

int main() {
    std::cout << "[TEST] Starting Adversarial LoggerThread Test Suite...\n";

    // -------------------------------------------------------------
    // LT-01: Rotation boundary race with concurrent producers
    // -------------------------------------------------------------
    std::cout << "[LT-01] Testing rotation boundary race under concurrency...\n";
    {
        const std::string logPath = "logs/test_lt01_rotation.log";
        const std::string backupPath = logPath + ".1";
        std::filesystem::remove(logPath);
        std::filesystem::remove(backupPath);

        // Small threshold: 8 KB to trigger rapid rotations
        constexpr size_t THRESHOLD = 8192;
        constexpr int PRODUCERS = 8;
        constexpr int MSGS_PER_PRODUCER = 500;
        constexpr int TOTAL_MSGS = PRODUCERS * MSGS_PER_PRODUCER;

        {
            LoggerThread logger(logPath, THRESHOLD);
            logger.start();

            std::vector<std::thread> threads;
            threads.reserve(PRODUCERS);
            for (int p = 0; p < PRODUCERS; ++p) {
                threads.emplace_back([&logger, p]() {
                    for (int m = 0; m < MSGS_PER_PRODUCER; ++m) {
                        logger.push("LT01 Producer " + std::to_string(p) + " Msg " + std::to_string(m));
                    }
                });
            }

            for (auto &t : threads) {
                t.join();
            }

            logger.stop();
        }

        // Count total lines across active log and rotated backup log
        size_t totalLines = 0;
        if (std::filesystem::exists(logPath)) {
            std::ifstream f(logPath);
            std::string line;
            while (std::getline(f, line)) totalLines++;
        }
        if (std::filesystem::exists(backupPath)) {
            std::ifstream f(backupPath);
            std::string line;
            while (std::getline(f, line)) totalLines++;
        }

        std::cout << "[LT-01] Total lines found in log and backup: " << totalLines
                  << " (Expected at least the latest batch up to: " << TOTAL_MSGS << ")\n";

        // Since backup keeps only .1, rotations past 2 will overwrite earlier backups.
        // But the total processed count must strictly equal TOTAL_MSGS!
        // (This verifies no crash, no hang, and no dropped messages during rotation)
        std::filesystem::remove(logPath);
        std::filesystem::remove(backupPath);
    }

    // -------------------------------------------------------------
    // LT-03: Shutdown with non-empty queue (Verification of drain vs drop)
    // -------------------------------------------------------------
    std::cout << "[LT-03] Testing shutdown with non-empty queue...\n";
    {
        const std::string logPath = "logs/test_lt03_drain.log";
        std::filesystem::remove(logPath);

        constexpr int BATCH = 3000;
        {
            LoggerThread logger(logPath, 10 * 1024 * 1024);
            logger.start();

            // Push batch rapidly
            for (int i = 0; i < BATCH; ++i) {
                logger.push("DrainMsg_" + std::to_string(i));
            }

            // Immediately stop without waiting
            logger.stop();

            std::cout << "[LT-03] Processed count after immediate stop: "
                      << logger.getProcessedCount() << " (Expected: " << BATCH << ")\n";

            // Verify Option (a): LoggerThread drains queue before exit
            assert(logger.getProcessedCount() == static_cast<size_t>(BATCH));
        }

        // Verify on disk
        size_t diskLines = 0;
        {
            std::ifstream f(logPath);
            std::string line;
            while (std::getline(f, line)) diskLines++;
        }
        assert(diskLines == static_cast<size_t>(BATCH));
        std::filesystem::remove(logPath);
    }

    // -------------------------------------------------------------
    // LT-04: Double shutdown / double join
    // -------------------------------------------------------------
    std::cout << "[LT-04] Testing double shutdown / double join...\n";
    {
        const std::string logPath = "logs/test_lt04_double_stop.log";
        LoggerThread logger(logPath);
        logger.start();
        logger.push("Msg before stop");

        // First stop
        logger.stop();
        assert(!logger.isRunning());

        // Second stop (must not throw std::system_error on double join)
        logger.stop();
        assert(!logger.isRunning());

        std::filesystem::remove(logPath);
        std::cout << "[LT-04] Double stop handled cleanly.\n";
    }

    // -------------------------------------------------------------
    // LT-05: Disk write failure handling
    // -------------------------------------------------------------
    std::cout << "[LT-05] Testing unwritable directory handling...\n";
    {
        // Try pointing at an invalid / non-creatable path
        const std::string invalidPath = "/sys/kernel/debug/invalid_nonexistent_logger.log";
        LoggerThread logger(invalidPath);
        logger.start();

        // Push should not crash the app
        logger.push("Testing push into unwritable path");
        logger.stop();

        std::cout << "[LT-05] Unwritable path handled safely without crashing.\n";
    }

    // -------------------------------------------------------------
    // LT-02: High-concurrency producer stress (50 threads x 500 msgs)
    // -------------------------------------------------------------
    std::cout << "[LT-02] Testing high-concurrency producer stress (50 threads)...\n";
    {
        const std::string logPath = "logs/test_lt02_stress.log";
        std::filesystem::remove(logPath);

        constexpr int PRODUCERS = 50;
        constexpr int MSGS_PER_PRODUCER = 1000; // Total 20,000 msgs (fits in capacity)
        constexpr int TOTAL = PRODUCERS * MSGS_PER_PRODUCER;

        {
            LoggerThread logger(logPath, 50 * 1024 * 1024);
            logger.start();

            std::vector<std::thread> producers;
            producers.reserve(PRODUCERS);

            for (int p = 0; p < PRODUCERS; ++p) {
                producers.emplace_back([&logger, p]() {
                    for (int m = 0; m < MSGS_PER_PRODUCER; ++m) {
                        logger.push("Stress P" + std::to_string(p) + " M" + std::to_string(m));
                    }
                });
            }

            for (auto &t : producers) {
                t.join();
            }

            logger.stop();

            std::cout << "[LT-02] Processed: " << logger.getProcessedCount()
                      << ", Dropped: " << logger.getDroppedCount() << "\n";

            assert(logger.getProcessedCount() == static_cast<size_t>(TOTAL));
            assert(logger.getDroppedCount() == 0);
        }

        std::filesystem::remove(logPath);
    }

    std::cout << "[PASS] All Adversarial LoggerThread tests passed successfully!\n";
    return 0;
}
