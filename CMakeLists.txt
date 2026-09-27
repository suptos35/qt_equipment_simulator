cmake_minimum_required(VERSION 3.16)
project(QtEquipmentSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets Network Test REQUIRED)

# Sanitizer configuration for concurrency testing
set(CMAKE_CXX_FLAGS_TSAN "-fsanitize=thread -g -O1")

enable_testing()
add_subdirectory(tests)

# Main executable target
add_executable(equipment_sim
    src/main.cpp
    src/Logging.cpp
    src/EquipmentState.cpp
    src/EquipmentWorker.cpp
    src/LoggerThread.cpp
    src/StateMachine.cpp
    src/GaugeWidget.cpp
    src/MainWindow.cpp
    src/MainWindow.ui
)

target_include_directories(equipment_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(equipment_sim PRIVATE
    Qt6::Widgets
    Qt6::Network
)
