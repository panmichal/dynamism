cmake_minimum_required(VERSION 3.22)

project(VelocityEnvelope VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(VelocityEnvelope
    COMPANY_NAME            "Dynamism"
    PLUGIN_NAME             "Velocity Envelope"
    PLUGIN_MANUFACTURER_CODE Dynm
    PLUGIN_CODE             DyVE
    FORMATS                 AU VST3
    PRODUCT_NAME            "Velocity Envelope"
    BUNDLE_ID               "com.dynamism.velocityenvelope"
    AU_MAIN_TYPE            kAudioUnitType_Effect
    AU_SANDBOX_SAFE         TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    HARDENED_RUNTIME_ENABLED TRUE
    VST3_CATEGORIES         "Fx" "Dynamics"
    DESCRIPTION             "Draw a multi-segment envelope to shape audio amplitude (velocity)"
)

juce_generate_juce_header(VelocityEnvelope)

target_sources(VelocityEnvelope PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/EnvelopeEditor.cpp
    Source/CustomWidgets.cpp
)

target_compile_definitions(VelocityEnvelope PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_HOST_CROSS_PLATFORM_UID=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(VelocityEnvelope PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
)

# Offscreen UI renderer (dev tool): renders the plugin editor to a PNG so the
# look can be checked without opening a host.
juce_add_console_app(UiRender PRODUCT_NAME "Velocity Envelope UI render")
juce_generate_juce_header(UiRender)
target_sources(UiRender PRIVATE
    tools/RenderUI.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/EnvelopeEditor.cpp
    Source/CustomWidgets.cpp
)
target_include_directories(UiRender PRIVATE Source)
target_compile_definitions(UiRender PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1
)
target_link_libraries(UiRender PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
)
