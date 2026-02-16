cmake_minimum_required(VERSION 3.15)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()

project(PluginHost VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Chugin name
set(CHUGIN_NAME PluginHost)

# Sources
set(SOURCES
    PluginHost.cpp
    PluginEditorWindow.cpp
    PluginHost.h
    CircularBuffer.h
    PlayHead.h
    PluginEditorWindow.h
    QWERTYMidiWindow.h
    Utilities.h
)

# Include directories
set(CK_SRC_PATH "../chuck/include")
set(JUCE_MODULES_PATH "Juce/8.0.4/modules")
set(JUCE_LIB_HEADERS "JuceStaticLib/JuceLibraryCode")

# Create the chugin (module)
add_library(${CHUGIN_NAME} MODULE ${SOURCES})

# Target properties
set_target_properties(${CHUGIN_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".chug"
    OUTPUT_NAME ${CHUGIN_NAME}
)

# Include directories
target_include_directories(${CHUGIN_NAME} PRIVATE
    ${CK_SRC_PATH}
    ${JUCE_MODULES_PATH}
    ${JUCE_LIB_HEADERS}
)

# Common definitions
target_compile_definitions(${CHUGIN_NAME} PRIVATE
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_STANDALONE_APPLICATION=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_PLUGINHOST_VST=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
)

if(MSVC)
    # Windows specific definitions
    target_compile_definitions(${CHUGIN_NAME} PRIVATE
        WIN32
        _WINDOWS
        __PLATFORM_WIN32__
        __WINDOWS_DS__
        JUCE_WINDOWS=1
    )

    # Linker settings for Windows
    set(JUCE_LIB_PATH "Juce/Win/Release/Static Library")
    
    target_link_directories(${CHUGIN_NAME} PRIVATE "${JUCE_LIB_PATH}")
    
    target_link_libraries(${CHUGIN_NAME} PRIVATE
        JuceStaticLib.lib
        user32.lib
        gdi32.lib
        winmm.lib
        shell32.lib
        advapi32.lib
        ole32.lib
        oleaut32.lib
        uuid.lib
        ws2_32.lib
        shlwapi.lib
        version.lib
        opengl32.lib
        setupapi.lib
        dwmapi.lib
        uxtheme.lib
        windowscodecs.lib
        rpcrt4.lib
        comdlg32.lib
        imm32.lib
        kernel32.lib
    )

elseif(APPLE)
    # macOS specific settings (based on makefile.mac)
    target_compile_definitions(${CHUGIN_NAME} PRIVATE
        __MACOSX_CORE__
        JUCE_MAC=1
        DLINUX=0
    )
    
    target_link_libraries(${CHUGIN_NAME} PRIVATE
        "-framework Cocoa"
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework IOKit"
        "-framework Accelerate"
        "-framework WebKit"
        "-framework DiscRecording"
        "-framework Foundation"
        "-framework QuartzCore"
        "-framework AudioToolbox"
        "-framework CoreAudioKit"
        "-framework Security"
    )
    
    # Static lib path for Mac
    target_link_directories(${CHUGIN_NAME} PRIVATE "Juce/Mac/Release")
    target_link_libraries(${CHUGIN_NAME} PRIVATE JuceStaticLib)

elseif(UNIX AND NOT APPLE)
    # Linux specific settings (based on makefile.linux)
    target_compile_definitions(${CHUGIN_NAME} PRIVATE
        __LINUX_ALSA__
        __PLATFORM_LINUX__
        JUCE_LINUX=1
    )
    
    # Linker settings for Linux
    # Note: Depending on JUCE modules used, more libraries might be needed here
    target_link_libraries(${CHUGIN_NAME} PRIVATE
        pthread
        dl
        rt
        stdc++
    )
    
    # Static lib path for Linux (if exists)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Juce/Linux/Release")
        target_link_directories(${CHUGIN_NAME} PRIVATE "Juce/Linux/Release")
        target_link_libraries(${CHUGIN_NAME} PRIVATE JuceStaticLib)
    endif()
endif()

# Post-build copy to root for all platforms
add_custom_command(TARGET ${CHUGIN_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:${CHUGIN_NAME}>"
    "${CMAKE_CURRENT_SOURCE_DIR}/"
)
