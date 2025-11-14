cmake_minimum_required(VERSION 3.16)
project(spring_uled_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to add explicit GDB debug information to compiler flags.
# When enabled this appends -g -ggdb to the C++ flags. You can disable with:
#   cmake -DENABLE_GDB_DEBUG_INFO=OFF ..
option(ENABLE_GDB_DEBUG_INFO "Add GDB debug information (-g -ggdb) to compiler flags" ON)

if(ENABLE_GDB_DEBUG_INFO)
    message(STATUS "ENABLE_GDB_DEBUG_INFO=ON: adding -g -ggdb to C++ flags")
    if(NOT CMAKE_CXX_FLAGS MATCHES "-g")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb")
    endif()
endif()

include(FetchContent)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(yaml-cpp)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.12.0
)
FetchContent_MakeAvailable(json)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod libgpiodcxx)

add_executable(spring_uledd
    src/daemon.cpp
    src/led_manager.cpp
    src/common.hpp
    src/led_manager.hpp
)

target_link_libraries(spring_uledd PRIVATE yaml-cpp nlohmann_json::nlohmann_json ${GPIOD_LIBRARIES})
target_include_directories(spring_uledd PRIVATE ${GPIOD_INCLUDE_DIRS})

add_executable(spring_uledctl
    src/client.cpp
    src/common.hpp
)
target_link_libraries(spring_uledctl PRIVATE yaml-cpp nlohmann_json::nlohmann_json)

install(TARGETS spring_uledd spring_uledctl
    RUNTIME DESTINATION /usr/bin
)

