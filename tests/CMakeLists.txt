add_executable(HandControlTests
    NormalizerTests.cpp
    OneEuroFilterTests.cpp
    HandIdentityTrackerTests.cpp
    ${CMAKE_SOURCE_DIR}/plugin/Source/Tracking/Normalizer.cpp
    ${CMAKE_SOURCE_DIR}/plugin/Source/Tracking/OneEuroFilter.cpp
    ${CMAKE_SOURCE_DIR}/plugin/Source/Tracking/HandIdentityTracker.cpp)

target_include_directories(HandControlTests PRIVATE
    ${CMAKE_SOURCE_DIR}/plugin/Source)

set_target_properties(HandControlTests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

if(MSVC)
    target_compile_options(HandControlTests PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(HandControlTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME HandControlTests COMMAND HandControlTests)
