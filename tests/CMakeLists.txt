# Unit & Adversarial Tests for Qt Equipment Simulator

find_package(Threads REQUIRED)

# ==============================================================================
# Base Test Suites
# ==============================================================================

# 1. EquipmentState unit test
add_executable(test_equipmentstate
    test_equipmentstate.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_equipmentstate PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_equipmentstate PRIVATE Qt6::Test Qt6::Core)
add_test(NAME test_equipmentstate COMMAND test_equipmentstate)

# 2. EquipmentWorker threading and concurrency test
add_executable(test_worker_threading
    test_worker_threading.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_worker_threading PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_worker_threading PRIVATE Qt6::Test Qt6::Core)
add_test(NAME test_worker_threading COMMAND test_worker_threading)

# 3. LoggerThread pure C++ multithreading test (no Qt dependencies)
add_executable(test_logger_thread
    test_logger_thread.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_logger_thread PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_logger_thread PRIVATE Threads::Threads)
add_test(NAME test_logger_thread COMMAND test_logger_thread)

# 4. StateMachine transition lifecycle and rejection test
add_executable(test_statemachine
    test_statemachine.cpp
    ${CMAKE_SOURCE_DIR}/src/StateMachine.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_statemachine PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_statemachine PRIVATE Qt6::Test Qt6::Core)
add_test(NAME test_statemachine COMMAND test_statemachine)

# 5. GUI Event Log queued connection test
add_executable(test_gui_log
    test_gui_log.cpp
    ${CMAKE_SOURCE_DIR}/src/MainWindow.cpp
    ${CMAKE_SOURCE_DIR}/src/MainWindow.ui
    ${CMAKE_SOURCE_DIR}/src/GaugeWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/StateMachine.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_gui_log PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_gui_log PRIVATE Qt6::Widgets Qt6::Test Qt6::Core)
add_test(NAME test_gui_log COMMAND test_gui_log)

# 6. SECS/GEM HSMS and SECS-II integration test
add_executable(test_secsgem
    test_secsgem.cpp
    ${CMAKE_SOURCE_DIR}/src/SecsServer.cpp
    ${CMAKE_SOURCE_DIR}/src/SecsClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_secsgem PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_secsgem PRIVATE Qt6::Network Qt6::Test Qt6::Core)
add_test(NAME test_secsgem COMMAND test_secsgem)

# ==============================================================================
# Adversarial Test Suites (SM, ES, WT, LT, SECS, LOG, INT)
# ==============================================================================

# Adv 1. StateMachine Adversarial Matrix (SM-01 to SM-06)
add_executable(test_adversarial_statemachine
    test_adversarial_statemachine.cpp
    ${CMAKE_SOURCE_DIR}/src/StateMachine.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_adversarial_statemachine PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_adversarial_statemachine PRIVATE Qt6::Test Qt6::Core)
add_test(NAME test_adversarial_statemachine COMMAND test_adversarial_statemachine)

# Adv 2. Physics & Bounds Adversarial (ES-01 to ES-04)
add_executable(test_adversarial_physics
    test_adversarial_physics.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_adversarial_physics PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_adversarial_physics PRIVATE Qt6::Test Qt6::Core)
add_test(NAME test_adversarial_physics COMMAND test_adversarial_physics)

# Adv 3. Threading Races & Lifecycle Adversarial (WT-01 to WT-04)
add_executable(test_adversarial_threading
    test_adversarial_threading.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_adversarial_threading PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_adversarial_threading PRIVATE Qt6::Test Qt6::Core)
add_test(NAME test_adversarial_threading COMMAND test_adversarial_threading)

# Adv 4. Logger Concurrency & Rotation Adversarial (LT-01 to LT-05) - Pure C++
add_executable(test_adversarial_logger
    test_adversarial_logger.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_adversarial_logger PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_adversarial_logger PRIVATE Threads::Threads)
add_test(NAME test_adversarial_logger COMMAND test_adversarial_logger)

# Adv 5. SECS/GEM HSMS Protocol Violations Adversarial (SECS-01 to SECS-05)
add_executable(test_adversarial_secsgem
    test_adversarial_secsgem.cpp
    ${CMAKE_SOURCE_DIR}/src/SecsServer.cpp
    ${CMAKE_SOURCE_DIR}/src/SecsClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_adversarial_secsgem PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_adversarial_secsgem PRIVATE Qt6::Network Qt6::Test Qt6::Core)
add_test(NAME test_adversarial_secsgem COMMAND test_adversarial_secsgem)

# Adv 6. Logging Pipeline & Filtering Adversarial (LOG-01 to LOG-03)
add_executable(test_adversarial_logging
    test_adversarial_logging.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_adversarial_logging PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_adversarial_logging PRIVATE Qt6::Test Qt6::Core)
add_test(NAME test_adversarial_logging COMMAND test_adversarial_logging)

# Adv 7. GUI Integration Adversarial (INT-01)
add_executable(test_adversarial_integration
    test_adversarial_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/MainWindow.cpp
    ${CMAKE_SOURCE_DIR}/src/MainWindow.ui
    ${CMAKE_SOURCE_DIR}/src/GaugeWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/StateMachine.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
    ${CMAKE_SOURCE_DIR}/src/LoggerThread.cpp
)
target_include_directories(test_adversarial_integration PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_adversarial_integration PRIVATE Qt6::Widgets Qt6::Test Qt6::Core)
add_test(NAME test_adversarial_integration COMMAND test_adversarial_integration)
