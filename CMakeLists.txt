cmake_minimum_required(VERSION 3.22)
project(WebPlug VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── JUCE ──────────────────────────────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/JUCE/CMakeLists.txt")
    add_subdirectory(modules/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        develop
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ── UI binary data (embedded in plugin build only) ────────────────────────────
juce_add_binary_data(WebPlugBinaryData
    NAMESPACE WebPlugBinaryData
    SOURCES
        ui/index.html
        ui/styles.css
        ui/bridge.js
        ui/main.js
)

# ── Plugin target (VST3 + AU) ─────────────────────────────────────────────────
juce_add_plugin(WebPlugPlugin
    COMPANY_NAME              "WebPlug"
    COMPANY_COPYRIGHT         "WebPlug 2026"
    PLUGIN_MANUFACTURER_CODE  Wplg
    PLUGIN_CODE               Wplg
    FORMATS                   VST3 AU
    PRODUCT_NAME              "WebPlug"
    BUNDLE_ID                 "io.webplug.plugin"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
)

juce_generate_juce_header(WebPlugPlugin)

target_sources(WebPlugPlugin PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
)

target_include_directories(WebPlugPlugin PRIVATE source)
target_compile_features(WebPlugPlugin PUBLIC cxx_std_17)

target_compile_definitions(WebPlugPlugin PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

target_compile_options(WebPlugPlugin PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-unused-parameter>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /wd4100>
)

target_link_libraries(WebPlugPlugin PRIVATE
    WebPlugBinaryData
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_warning_flags
)

# ── Dev server target (standalone WebSocket server) ───────────────────────────
juce_add_console_app(WebPlugServer PRODUCT_NAME "WebPlugServer")

juce_generate_juce_header(WebPlugServer)

target_sources(WebPlugServer PRIVATE
    source/AudioEngine.cpp
    source/DevServer.cpp
    source/ServerMain.cpp
)

target_include_directories(WebPlugServer PRIVATE source)
target_compile_features(WebPlugServer PUBLIC cxx_std_17)

target_compile_definitions(WebPlugServer PRIVATE
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

target_compile_options(WebPlugServer PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wno-unused-parameter>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /wd4100>
)

target_link_libraries(WebPlugServer PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_recommended_warning_flags
)
