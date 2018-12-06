# usage:
#
# project(example)
# include(Dependency.cmake)
# add_executable(example main.cpp)
# add_dependencies(example ${DEP_PROJECTS})
# target_include_directories(example PUBLIC ${DEP_INSTALL_DIR}/include)
# target_link_libraries(example PUBLIC -L${DEP_INSTALL_DIR}/lib) # add what you want

include(ExternalProject)

set(DEP_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Dependency/${CMAKE_BUILD_TYPE})
set(DEP_PROJECTS dep_fmt dep_spdlog dep_rttr dep_lua dep_sol)

ExternalProject_Add(
    dep_fmt
    GIT_REPOSITORY "https://github.com/fmtlib/fmt"
    GIT_TAG "5.2.1"
    GIT_SHALLOW 1
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${DEP_INSTALL_DIR}
        -DFMT_DOC=OFF
        -DFMT_TEST=OFF
	TEST_COMMAND ""
)

ExternalProject_Add(
    dep_rttr
    GIT_REPOSITORY "https://github.com/rttrorg/rttr"
    GIT_TAG "v0.9.6"
    GIT_SHALLOW 1
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${DEP_INSTALL_DIR}
        -DBUILD_RTTR_DYNAMIC=OFF
        -DBUILD_UNIT_TESTS=OFF
        -DBUILD_EXAMPLES=OFF
        -DBUILD_STATIC=ON
        -DBUILD_DOCUMENTATION=OFF
        -DBUILD_INSTALLER=ON
        -DBUILD_PACKAGE=OFF
    TEST_COMMAND ""
)

ExternalProject_Add(
    dep_lua
    GIT_REPOSITORY "https://github.com/LuaDist/lua"
    GIT_TAG "lua-5.3"
    GIT_SHALLOW 1
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${DEP_INSTALL_DIR}
        -DLUA_BUILD_EXECUTABLE=OFF
        -DBUILD_SHARED_LIBS=OFF
    TEST_COMMAND ""
)

ExternalProject_Add(
    dep_sol
    GIT_REPOSITORY "https://github.com/ThePhD/sol2"
    GIT_TAG "v2.20.6"
    GIT_SHALLOW 1
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${DEP_INSTALL_DIR}
    TEST_COMMAND ""
)

ExternalProject_Add(
    dep_spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog"
    GIT_TAG "v1.x"
    GIT_SHALLOW 1
    UPDATE_COMMAND ""
    PATCH_COMMAND ""
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${DEP_INSTALL_DIR}
        -DSPDLOG_BUILD_EXAMPLES=OFF
        -DSPDLOG_BUILD_BENCH=OFF
        -DSPDLOG_BUILD_TESTS=OFF
    TEST_COMMAND ""
)