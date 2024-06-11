cmake_minimum_required(VERSION 3.10)

# Название проекта
project(BlumAutoPilot)

# Укажите стандарт C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Найдите пакеты OpenCV
find_package(OpenCV REQUIRED)
find_package(X11 REQUIRED)
find_library(XI_LIB Xi)

# Укажите исполняемый файл и его исходники
add_executable(BlumAutoPilot image_parser.cpp)

# Свяжите исполняемый файл с библиотеками OpenCV и X11
target_link_libraries(BlumAutoPilot ${OpenCV_LIBS} ${X11_LIBRARIES} ${XI_LIB} Xtst)
include_directories(${X11_INCLUDE_DIR})
