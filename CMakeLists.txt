cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "extra_components/q/q_lib" "extra_components/q-infra")

# Use a version number in the project binary
set(PROJECT_VER "1.0.4")
set(PROJECT_PREF "q-tune")
set(PROJECT_BIN "${PROJECT_PREF}-${PROJECT_VER}")
project("${PROJECT_BIN}")

link_libraries(PUBLIC libq)
link_libraries(PUBLIC infra)
