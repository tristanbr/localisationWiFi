cmake_minimum_required (VERSION 2.6)
project (APcapture)

set(CMAKE_CXX_STANDARD 11)

include_directories(./include)
include(FindPCAP.cmake)

add_executable(APcapture src/main.cpp src/localisation.cpp)
target_link_libraries(APcapture ${PCAP_LIBRARY})
