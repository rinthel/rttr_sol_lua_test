cmake_minimum_required(VERSION 3.7)

include(Dependency.cmake)

project(rttr_sol_lua_test)
set(CMAKE_CXX_STANDARD 14)

add_executable(rttr_sol_lua_test main.cpp)
add_dependencies(rttr_sol_lua_test ${DEP_PROJECTS})
target_include_directories(rttr_sol_lua_test PUBLIC ${DEP_INSTALL_DIR}/include)
target_link_libraries(rttr_sol_lua_test PUBLIC
    -L${DEP_INSTALL_DIR}/lib
    rttr_core lua fmt) # add what you want
