cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
set(CMAKE_CXX_STANDARD 23)

project(simple-coin)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED COMPONENTS
        system
        program_options
)
include_directories(${Boost_INCLUDE_DIRS})

set(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

include_directories(src)

set(SRC
    src/main.cpp
)

add_executable(simple-coin ${SRC})
target_link_libraries(simple-coin ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES})
