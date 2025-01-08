cmake_minimum_required(VERSION 3.15)

project(sub VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the source directory for sub
set(SUB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)

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
    set(SUB_LINK_FLAGS "-Wl,--no-undefined -Wl,--version-script=${SUB_SRC}/plugin.version")

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
    set(SUB_LINK_FLAGS "")
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
    set(SUB_LINK_FLAGS "-Wl,--no-undefined -Wl,--version-script=${SUB_SRC}/plugin.version")
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

# Add signals-unity-bridge shared library
add_library(signals-unity-bridge SHARED
    ${SUB_SRC}/plugin.cpp
)

target_include_directories(signals-unity-bridge
    PRIVATE ${SUB_SRC}
    PRIVATE ${CMAKE_SOURCE_DIR}/signals/include
)

target_link_libraries(signals-unity-bridge
    PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,signals::media>"
    PRIVATE signals::modules
    PRIVATE signals::pipeline
    PRIVATE signals::utils
)

#set_target_properties(signals-unity-bridge PROPERTIES
#    OUTPUT_NAME "signals-unity-bridge"
#    PREFIX ""
#    SUFFIX ".so"
#    LINK_FLAGS "${SUB_LINK_FLAGS}"
#)

# Install signals-unity-bridge shared library
install(TARGETS signals-unity-bridge
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Add loader executable
add_executable(loader
    ${SUB_SRC}/loader.cpp
    ${SUB_SRC}/dynlib_${HOST}.cpp
)

target_include_directories(loader
    PRIVATE ${SUB_SRC}
    PRIVATE ${CMAKE_SOURCE_DIR}/signals/include
)

target_link_libraries(loader
    PRIVATE signals::appcommon
    PRIVATE signals::utils
    PRIVATE signals-unity-bridge
)

set_target_properties(loader PROPERTIES
    OUTPUT_NAME "loader"
    SUFFIX ".exe"
)

# Add example executable
add_executable(example
    ${SUB_SRC}/example.cpp
)

target_include_directories(example
    PRIVATE ${SUB_SRC}
    PRIVATE ${CMAKE_SOURCE_DIR}/signals/include
)

target_link_libraries(example
    PRIVATE signals::appcommon
    PRIVATE signals::utils
    PRIVATE signals-unity-bridge
)

set_target_properties(example PROPERTIES
    OUTPUT_NAME "example"
    SUFFIX ".exe"
)

# Install loader executable
install(TARGETS loader
    RUNTIME DESTINATION bin
)

# Install example executable
install(TARGETS example
    RUNTIME DESTINATION bin
)


