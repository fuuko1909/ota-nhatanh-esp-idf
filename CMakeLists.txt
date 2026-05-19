# Cho phep dung lam top-level project hoac component.
cmake_minimum_required(VERSION 3.16)

if(NOT CMAKE_PROJECT_NAME)
  set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/components")
  include($ENV{IDF_PATH}/tools/cmake/project.cmake)
  project(ota_nhatanh_lib)
endif()
