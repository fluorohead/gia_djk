cmake_minimum_required(VERSION 3.5)

project(djk_ipv4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(djk_ipv4 main.cpp
    gia_djk.h gia_djk.cpp

    ../gia_ipmnp/gia_ipmnp.h
    ../gia_ipmnp/gia_ipmnp.cpp


)

include(GNUInstallDirs)
install(TARGETS djk_ipv4
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
