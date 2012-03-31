cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules/)
find_package(OpenHRP REQUIRED)
include_directories(${OPENHRP_INCLUDE_DIRS})
link_directories(${OPENHRP_LIBRARY_DIRS})
add_definitions(${OPENHRP_DEFINITIONS})

include(FindPkgConfig)
pkg_check_modules(OpenCV opencv)
include_directories(${OpenCV_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

set(target glutolv)

add_executable(glutolv GLmodel.cpp OnlineViewer_impl.cpp glutolv.cpp)
target_link_libraries(glutolv ${OPENHRP_LIBRARIES} ${OpenCV_LIBRARIES} ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

install(TARGETS ${target}
  RUNTIME DESTINATION bin
)
