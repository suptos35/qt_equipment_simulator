# Unit Tests for Qt Equipment Simulator

find_package(Threads REQUIRED)

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
