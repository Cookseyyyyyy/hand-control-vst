add_executable(HandControlTests
    NormalizerTests.cpp
    OneEuroFilterTests.cpp
    HandIdentityTrackerTests.cpp
    RoiTests.cpp
    MidiCcSenderTests.cpp
    ${CMAKE_SOURCE_DIR}/plugin/Source/Tracking/Normalizer.cpp
    ${CMAKE_SOURCE_DIR}/plugin/Source/Tracking/OneEuroFilter.cpp
    ${CMAKE_SOURCE_DIR}/plugin/Source/Tracking/HandIdentityTracker.cpp
    ${CMAKE_SOURCE_DIR}/plugin/Source/Tracking/Roi.cpp
    ${CMAKE_SOURCE_DIR}/plugin/Source/Midi/MidiCcSender.cpp)

target_include_directories(HandControlTests PRIVATE
    ${CMAKE_SOURCE_DIR}/plugin/Source)

# MidiCcSender depends on juce::MidiBuffer / MidiMessage, so link the audio
# basics module. JUCE is already fetched at root, so this is a cheap add.
target_link_libraries(HandControlTests PRIVATE
    juce::juce_audio_basics)

target_compile_definitions(HandControlTests PRIVATE
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_STANDALONE_APPLICATION=1)

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
