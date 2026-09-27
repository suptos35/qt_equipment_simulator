# Unit Tests for Qt Equipment Simulator

add_executable(test_equipmentstate
    test_equipmentstate.cpp
    ${CMAKE_SOURCE_DIR}/src/EquipmentState.cpp
    ${CMAKE_SOURCE_DIR}/src/Logging.cpp
)

target_include_directories(test_equipmentstate PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_equipmentstate PRIVATE
    Qt6::Test
    Qt6::Core
)

add_test(NAME test_equipmentstate COMMAND test_equipmentstate)
