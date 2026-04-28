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
    # We disable JUCE's built-in COPY_PLUGIN_AFTER_BUILD and handle installation
    # ourselves so that our runtime DLL ends up in the bundle *before* the copy
    # runs. See handcontrol_copy_runtime_to() below.
    COPY_PLUGIN_AFTER_BUILD    FALSE
    HARDENED_RUNTIME_ENABLED   TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    CAMERA_PERMISSION_ENABLED  TRUE
    CAMERA_PERMISSION_TEXT     "Hand Control needs the camera to track hand gestures."
    MICROPHONE_PERMISSION_TEXT "Hand Control does not use the microphone."
    APP_SANDBOX_ENABLED        FALSE)

juce_generate_juce_header(HandControlPlugin)

# Bundle the ONNX hand-tracking models directly into the plugin binary.
# End users never see these files; they're embedded as binary data and read
# from memory at plugin load time.
juce_add_binary_data(HandControlModels SOURCES
    "${HANDCONTROL_PALM_MODEL}"
    "${HANDCONTROL_HANDPOSE_MODEL}"
    HEADER_NAME HandControlModels.h
    NAMESPACE handcontrol::models)

set_target_properties(HandControlModels PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    FOLDER "HandControl")

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
    Source/Tracking/OnnxHandTracker.cpp
    Source/Tracking/OnnxHandTracker.h
    Source/Tracking/PalmAnchors.cpp
    Source/Tracking/PalmAnchors.h
    Source/Tracking/ImagePreprocess.cpp
    Source/Tracking/ImagePreprocess.h
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

target_include_directories(HandControlPlugin PRIVATE
    Source
    "${ONNXRUNTIME_INCLUDE_DIR}")

target_compile_definitions(HandControlPlugin PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_CAMERA=1
    JUCE_MODAL_LOOPS_PERMITTED=0
    HANDCONTROL_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(HandControlPlugin PRIVATE
    HandControlModels
    "${ONNXRUNTIME_IMPORT_LIBRARIES}"
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

set_target_properties(HandControlPlugin PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

if(WIN32)
    # Tell the Windows loader to search the DLL's own directory (inside the
    # VST3 bundle) when resolving onnxruntime.dll. Without this, any process
    # that LoadLibraryWs our VST3 - including juce_vst3_helper, Ableton, and
    # DAW plugin scanners - would search their own directory instead and
    # fail to find the runtime.
    # /DEPENDENTLOADFLAG:0x100 == LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
    target_link_options(HandControlPlugin PRIVATE "/DEPENDENTLOADFLAG:0x100")
endif()

# -----------------------------------------------------------------------------
# Bundle the ONNX Runtime shared library alongside the plugin so it loads at
# runtime without any user-visible setup.
# -----------------------------------------------------------------------------

# On Windows, JUCE's VST3 post-build step runs `juce_vst3_helper.exe -create`
# which LoadLibraryW's our .vst3 to generate moduleinfo.json. Windows' DLL
# search looks for onnxruntime.dll next to the helper (not next to the VST3),
# so we also copy the runtime there once the helper has been built.
if(WIN32 AND TARGET juce_vst3_helper)
    foreach(rt ${ONNXRUNTIME_RUNTIME_LIBRARIES})
        add_custom_command(TARGET juce_vst3_helper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${rt}"
                "$<TARGET_FILE_DIR:juce_vst3_helper>"
            COMMENT "Staging ONNX Runtime next to juce_vst3_helper")
    endforeach()
endif()

function(handcontrol_stage_runtime_into_bundle target)
    if(NOT TARGET ${target})
        return()
    endif()

    if(WIN32)
        # We want the DLL in place BEFORE juce_vst3_helper.exe runs, which
        # happens at VST3 post-build. Attach to PRE_LINK so the DLL lands next
        # to the module ahead of JUCE's post-build scan.
        # (TARGET_FILE_DIR is valid at PRE_LINK time for shared libs in
        # multi-config generators.)
        foreach(rt ${ONNXRUNTIME_RUNTIME_LIBRARIES})
            add_custom_command(TARGET ${target} PRE_LINK
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${target}>"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${rt}"
                    "$<TARGET_FILE_DIR:${target}>"
                COMMENT "Staging ONNX Runtime into ${target} output dir")
        endforeach()

    elseif(APPLE)
        # Bundle layout: <Name>.vst3/Contents/MacOS/<Name>
        # Place dylib at <Name>.vst3/Contents/Resources/ and set rpath.
        foreach(rt ${ONNXRUNTIME_RUNTIME_LIBRARIES})
            get_filename_component(_rt_name "${rt}" NAME)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_BUNDLE_DIR:${target}>/Contents/Resources"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${rt}"
                    "$<TARGET_BUNDLE_DIR:${target}>/Contents/Resources/${_rt_name}"
                COMMENT "Staging ONNX Runtime ${_rt_name} into bundle Resources")
        endforeach()

        set_target_properties(${target} PROPERTIES
            INSTALL_RPATH "@loader_path/../Resources"
            BUILD_WITH_INSTALL_RPATH TRUE)
    endif()
endfunction()

foreach(_fmt VST3 AU Standalone)
    set(_sub_target HandControlPlugin_${_fmt})
    if(TARGET ${_sub_target})
        handcontrol_stage_runtime_into_bundle(${_sub_target})
    endif()
endforeach()

# -----------------------------------------------------------------------------
# Local install (equivalent of JUCE's COPY_PLUGIN_AFTER_BUILD, but runs AFTER
# we've staged the runtime DLL so the installed bundle is complete).
# Disabled in CI via HANDCONTROL_COPY_PLUGIN_AFTER_BUILD.
# -----------------------------------------------------------------------------
if(HANDCONTROL_COPY_PLUGIN_AFTER_BUILD AND TARGET HandControlPlugin_VST3)
    if(WIN32)
        # VST3 bundle layout: <artefact>/VST3/Hand Control.vst3/Contents/x86_64-win/<DLL>
        # TARGET_FILE_DIR points at .../x86_64-win, so the bundle root is two up.
        set(_HC_VST3_BUNDLE "$<TARGET_FILE_DIR:HandControlPlugin_VST3>/../..")
        set(_HC_VST3_INSTALL_DIR "$ENV{CommonProgramFiles}/VST3")
        add_custom_command(TARGET HandControlPlugin_VST3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_HC_VST3_INSTALL_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${_HC_VST3_BUNDLE}"
                "${_HC_VST3_INSTALL_DIR}/Hand Control.vst3"
            COMMENT "Installing Hand Control.vst3 to ${_HC_VST3_INSTALL_DIR}")

    elseif(APPLE)
        set(_HC_VST3_INSTALL_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")
        add_custom_command(TARGET HandControlPlugin_VST3 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_HC_VST3_INSTALL_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "$<TARGET_BUNDLE_DIR:HandControlPlugin_VST3>"
                "${_HC_VST3_INSTALL_DIR}/Hand Control.vst3"
            COMMENT "Installing Hand Control.vst3 to ${_HC_VST3_INSTALL_DIR}")

        if(TARGET HandControlPlugin_AU)
            set(_HC_AU_INSTALL_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
            add_custom_command(TARGET HandControlPlugin_AU POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${_HC_AU_INSTALL_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "$<TARGET_BUNDLE_DIR:HandControlPlugin_AU>"
                    "${_HC_AU_INSTALL_DIR}/Hand Control.component"
                COMMENT "Installing Hand Control.component to ${_HC_AU_INSTALL_DIR}")
        endif()
    endif()
endif()
