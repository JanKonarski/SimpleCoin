cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
set(CMAKE_CXX_STANDARD 23)

project(miner)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED COMPONENTS
        system
        program_options
)
include_directories(${Boost_INCLUDE_DIRS})

#set(OPENSSL_USE_STATIC_LIBS ON)
#find_package(OpenSSL REQUIRED)
#include_directories(${OPENSSL_INCLUDE_DIR})

find_package(cryptopp REQUIRED static)
#include_directories(${CRYPTOPP_INCLUDE_DIR})
include_directories(/usr/local/include/cryptopp)

include_directories(src)



set(SRC
    src/miner/main.cpp
)

add_executable(miner ${SRC})
target_link_libraries(miner ${Boost_LIBRARIES} /usr/local/lib/libcryptopp.a)
# ${cryptopp_LIBRARIES}
# ${OPENSSL_LIBRARIES}



project(client)

set(SRC
    src/client/main.cpp
)

add_executable(client ${SRC})
#target_link_libraries(client ${Boost_LIBRARIES} ${cryptopp_LIBRARIES})
