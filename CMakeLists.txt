cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "extra_components/q/q_lib" "extra_components/q-infra")

project(q-tune)

link_libraries(PUBLIC libq)
link_libraries(PUBLIC infra)
