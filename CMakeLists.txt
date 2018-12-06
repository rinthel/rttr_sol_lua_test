cmake_minimum_required(VERSION 3.7)

include(Dependency.cmake)

project(sol_example)
set(CMAKE_CXX_STANDARD 14)

add_executable(sol_example main.cpp)
add_dependencies(sol_example ${DEP_PROJECTS})
target_include_directories(sol_example PUBLIC ${DEP_INSTALL_DIR}/include)
target_link_libraries(sol_example PUBLIC
    -L${DEP_INSTALL_DIR}/lib
    rttr_core lua fmt) # add what you want
