juce_add_plugin(HandControlPlugin
    PRODUCT_NAME               "Hand Control"
    COMPANY_NAME               "HandControl"
    BUNDLE_ID                  "com.handcontrol.HandControl"
    PLUGIN_MANUFACTURER_CODE   Hctl
    PLUGIN_CODE                Hctl
    FORMATS                    VST3 AU Standalone
    VST3_CATEGORIES            "Fx" "Tools"
    AU_MAIN_TYPE               "kAudioUnitType_Effect"
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD    ${HANDCONTROL_COPY_PLUGIN_AFTER_BUILD}
    HARDENED_RUNTIME_ENABLED   TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    CAMERA_PERMISSION_ENABLED  TRUE
    CAMERA_PERMISSION_TEXT     "Hand Control needs the camera to track hand gestures."
    MICROPHONE_PERMISSION_TEXT "Hand Control does not use the microphone."
    APP_SANDBOX_ENABLED        FALSE)

juce_generate_juce_header(HandControlPlugin)

target_sources(HandControlPlugin PRIVATE
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginProcessor.h
    Source/Plugin/PluginEditor.cpp
    Source/Plugin/PluginEditor.h
    Source/Params/ParameterIDs.h
    Source/Params/ParameterLayout.cpp
    Source/Params/ParameterLayout.h
    Source/Params/ParameterBridge.cpp
    Source/Params/ParameterBridge.h
    Source/Tracking/IHandTracker.h
    Source/Tracking/Landmarks.h
    Source/Tracking/MockHandTracker.cpp
    Source/Tracking/MockHandTracker.h
    Source/Tracking/HandTrackerThread.cpp
    Source/Tracking/HandTrackerThread.h
    Source/Tracking/CameraSource.cpp
    Source/Tracking/CameraSource.h
    Source/Tracking/Normalizer.cpp
    Source/Tracking/Normalizer.h
    Source/Tracking/OneEuroFilter.cpp
    Source/Tracking/OneEuroFilter.h
    Source/Tracking/HandIdentityTracker.cpp
    Source/Tracking/HandIdentityTracker.h
    Source/UI/PreviewComponent.cpp
    Source/UI/PreviewComponent.h
    Source/UI/ParameterMeter.cpp
    Source/UI/ParameterMeter.h
    Source/UI/Theme.cpp
    Source/UI/Theme.h
    Source/UI/StatusBar.cpp
    Source/UI/StatusBar.h)

target_include_directories(HandControlPlugin PRIVATE Source)

target_compile_definitions(HandControlPlugin PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_CAMERA=1
    JUCE_MODAL_LOOPS_PERMITTED=0
    HANDCONTROL_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(HandControlPlugin PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_video
    juce::juce_opengl
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

if(HANDCONTROL_ENABLE_TFLITE)
    target_sources(HandControlPlugin PRIVATE
        Source/Tracking/TFLiteHandTracker.cpp
        Source/Tracking/TFLiteHandTracker.h)
    target_compile_definitions(HandControlPlugin PRIVATE HANDCONTROL_HAS_TFLITE=1)
    # See docs/MEDIAPIPE_INTEGRATION.md for how to provide TFLITE_ROOT.
    if(NOT DEFINED TFLITE_ROOT)
        message(FATAL_ERROR "HANDCONTROL_ENABLE_TFLITE=ON but TFLITE_ROOT is not set. "
                            "See docs/MEDIAPIPE_INTEGRATION.md.")
    endif()
    target_include_directories(HandControlPlugin PRIVATE "${TFLITE_ROOT}/include")
    target_link_directories(HandControlPlugin PRIVATE "${TFLITE_ROOT}/lib")
    target_link_libraries(HandControlPlugin PRIVATE tensorflowlite_c)
endif()

set_target_properties(HandControlPlugin PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
