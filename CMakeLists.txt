cmake_minimum_required(VERSION 3.15)

project(lldplay VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the source directory for sub
set(LLDPLAY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Determine the host system and set the appropriate source file
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(HOST "gnu")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(HOST "mingw")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(HOST "darwin")
else()
    message(FATAL_ERROR "Unsupported host system: ${CMAKE_SYSTEM_NAME}")
endif()


# Change some compiler options that are dependent on the target platform.
# xxxjack some are actually dependent on the toolchain...
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_compile_options(-Wall -Wextra -Werror -fvisibility=default -fvisibility-inlines-hidden -Wno-deprecated-declarations)
    set(LLDPLAY_LINK_FLAGS "-Wl,--no-undefined -Wl,--version-script=${LLDPLAY_SRC}/plugin.version")

    # Debug-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3)
    endif()

    # Release-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-w -DNDEBUG)
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    add_compile_options(-Wall -Wextra -Werror -fvisibility=default -fvisibility-inlines-hidden -Wno-deprecated-declarations)
    set(LLDPLAY_LINK_FLAGS "")
    # Debug-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3)
    endif()

    # Release-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-w -DNDEBUG)
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_compile_options(-Wall -Wextra -Werror -fvisibility=default -fvisibility-inlines-hidden -Wno-deprecated-declarations)
    set(LLDPLAY_LINK_FLAGS "-Wl,--no-undefined -Wl,--version-script=${LLDPLAY_SRC}/plugin.version")
    # Debug-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g3)
    endif()

    # Release-specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-w -DNDEBUG)
    endif()
else()
    message(FATAL_ERROR " Unknown CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
endif()

# Add lldash_play shared library
add_library(lldash_play SHARED
    ${LLDPLAY_SRC}/plugin.cpp
)

target_include_directories(lldash_play
    PRIVATE ${LLDPLAY_SRC}
    PRIVATE ${CMAKE_SOURCE_DIR}/signals/include
)

target_link_libraries(lldash_play
    PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,signals::media>"
    PRIVATE signals::modules
    PRIVATE signals::pipeline
    PRIVATE signals::utils
)

set_target_properties(lldash_play PROPERTIES
    OUTPUT_NAME "lldash_play"
    PREFIX ""
    SUFFIX ".so"
    LINK_FLAGS "${LLDPLAY_LINK_FLAGS}"
)

signals_install_plugin(lldash_play ".so")

# Add loader executable
add_executable(loader
    ${LLDPLAY_SRC}/loader.cpp
    ${LLDPLAY_SRC}/dynlib_${HOST}.cpp
)

target_include_directories(loader
    PRIVATE ${LLDPLAY_SRC}
    PRIVATE ${CMAKE_SOURCE_DIR}/signals/include
)

target_link_libraries(loader
    PRIVATE signals::appcommon
    PRIVATE signals::utils
    PRIVATE lldash_play
)

signals_install_app(loader)

# Add example executable
add_executable(example
    ${LLDPLAY_SRC}/example.cpp
)

target_include_directories(example
    PRIVATE ${LLDPLAY_SRC}
    PRIVATE ${CMAKE_SOURCE_DIR}/signals/include
)

target_link_libraries(example
    PRIVATE signals::appcommon
    PRIVATE signals::utils
    PRIVATE lldash_play
)

signals_install_app(example)
