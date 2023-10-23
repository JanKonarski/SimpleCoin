cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 23)

project(simple-coin)

find_package(Boost REQUIRED COMPONENTS
        system
        program_options
)
include_directories(${Boost_INCLUDE_DIRS})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

include_directories(src)

set(SRC
    src/main.cpp
)

add_executable(simple-coin ${SRC})
target_link_libraries(simple-coin ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES})
