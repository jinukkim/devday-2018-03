cmake_minimum_required(VERSION 2.8)
project (flaskr_cpp)

find_package(Threads)
find_package(OpenSSL)

if(OPENSSL_FOUND)
	include_directories(${OPENSSL_INCLUDE_DIR})
endif()


include_directories(/usr/include/mariadb)
find_library(libmariadb libmariadb.so PATHS /usr/lib/mariadb /usr/lib64/mariadb REQUIRED)
find_library(libjemalloc libjemalloc.so REQUIRED)


find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  message("Found ccache ${CCACHE_FOUND}")
  message("Using ccache to speed up compilation")
  set(ENV{CCACHE_CPP2} "yes")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1y -pedantic -Wextra")
find_package(Boost 1.52 COMPONENTS system thread REQUIRED)

include_directories(${Boost_INCLUDE_DIR})

set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

include_directories("${PROJECT_INCLUDE_DIR}")


add_executable(flaskr flask.cc)
target_link_libraries(
    flaskr
    ${libjemalloc}
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${libmariadb})
