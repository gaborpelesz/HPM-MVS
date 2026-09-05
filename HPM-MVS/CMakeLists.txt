cmake_minimum_required (VERSION 2.8)
project (HPM)

set(CMAKE_CXX_STANDARD 20)

include(CheckLanguage)
check_language(CUDA)
if (CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    message(" -- CUDA FOUND")
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3 --use_fast_math --maxrregcount=128 --ptxas-options=-v --compiler-options -Wall")
    # set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -lineinfo") # for CUDA profiling!
else()
    message(FATAL_ERROR "-- CUDA NOT FOUND: Fatal error")
endif()


find_package(OpenCV REQUIRED)
find_package(PCL 1.7 REQUIRED)

include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})


if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-pthread)
    add_definitions(-Wall)
    add_definitions(-Wextra)
    add_definitions(-pedantic)
    add_definitions(-Wno-unused-function)
    add_definitions(-Wno-switch)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffast-math -march=native") # extend release-profile with fast-math
endif()

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()


# For compilation ...
# Specify target & source files to compile it from
add_executable(
    HPM
    main.h
    HPM.h
    HPM.cpp
    HPM.cu
    main.cpp
)

# For linking ...
# Specify target & libraries to link it with
target_link_libraries(HPM
    ${OpenCV_LIBS}
    ${PCL_LIBRARIES}
    )

set_target_properties(HPM
    PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
target_include_directories(HPM
    PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
)
