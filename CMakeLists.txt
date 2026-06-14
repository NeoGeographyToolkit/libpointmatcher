cmake_minimum_required(VERSION 3.15)

include(CheckSymbolExists)

project(libpointmatcher)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)

# Extract version from header
execute_process(
	COMMAND grep "POINTMATCHER_VERSION " pointmatcher/PointMatcher.h
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	RESULT_VARIABLE GREP_VERSION_RESULT
	OUTPUT_VARIABLE PROJECT_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (NOT GREP_VERSION_RESULT EQUAL 0)
	message(SEND_ERROR "Cannot grep version number: ${GREP_VERSION_RESULT}")
endif()
string(REGEX REPLACE ".*\"(.*)\".*" "\\1" PROJECT_VERSION "${PROJECT_VERSION}")

# Require 64-bit platform
if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(FATAL_ERROR "Libpointmatcher requires a 64-bit platform.")
endif()

# Installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")

# Make relative paths absolute
foreach(p LIB BIN INCLUDE)
	set(var INSTALL_${p}_DIR)
	if (NOT IS_ABSOLUTE "${${var}}")
		set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
	endif()
endforeach()

# Dependencies
set(EXTERNAL_LIBS "")

# Boost
find_package(Boost COMPONENTS thread filesystem system program_options date_time chrono REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
set(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${Boost_LIBRARIES})

# Eigen
find_package(Eigen3 3 REQUIRED NO_MODULE)
include_directories(${EIGEN3_INCLUDE_DIR})
message(STATUS "Found Eigen3: ${EIGEN3_INCLUDE_DIRS}")

# libnabo
find_library(NABO_LIBRARY NAMES nabo REQUIRED)
find_path(NABO_INCLUDE_DIR nabo/nabo.h REQUIRED)
include_directories(${NABO_INCLUDE_DIR})
set(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${NABO_LIBRARY})
message(STATUS "Found libnabo: ${NABO_LIBRARY}")

# OpenMP
if (APPLE)
	set(OPENMP_ROOT "$ENV{CONDA_PREFIX}")
	find_package(OpenMP)
	set(OPENMP_INCLUDES "${OPENMP_ROOT}/include")
	set(OpenMP_C_LIB_NAMES "omp")
	set(OpenMP_CXX_LIB_NAMES "omp")
	set(OpenMP_C_FLAGS "-fopenmp")
	set(OpenMP_CXX_FLAGS "-fopenmp")
	set(OpenMP_omp_LIBRARY "${OPENMP_ROOT}/lib/libomp${CMAKE_SHARED_LIBRARY_SUFFIX}")
	set(OpenMP_EXE_LINKER_FLAGS "${OpenMP_omp_LIBRARY}")
	set(OpenMP_CXX_FOUND ON)
	message(STATUS "Manually set: OpenMP_CXX_FLAGS=${OpenMP_CXX_FLAGS}")
else()
	find_package(OpenMP REQUIRED)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

# POSIX timers (Linux)
check_symbol_exists(_POSIX_TIMERS "unistd.h;time.h" POSIX_TIMERS)
if (POSIX_TIMERS AND NOT APPLE)
	set(EXTERNAL_LIBS ${EXTERNAL_LIBS} rt)
endif()

# Source files
set(POINTMATCHER_SRC
	pointmatcher/Logger.cpp
	pointmatcher/Exceptions.cpp
	pointmatcher/DataPoints.cpp
	pointmatcher/Matches.cpp
	pointmatcher/ICP.cpp
	pointmatcher/Registry.cpp
	pointmatcher/Registrar.cpp
	pointmatcher/DataPointsFilter.cpp
	pointmatcher/Matcher.cpp
	pointmatcher/OutlierFilter.cpp
	pointmatcher/ErrorMinimizer.cpp
	pointmatcher/Transformation.cpp
	pointmatcher/TransformationChecker.cpp
	pointmatcher/Inspector.cpp
	pointmatcher/IO.cpp
	pointmatcher/Timer.cpp
	pointmatcher/Histogram.cpp
	pointmatcher/Parametrizable.cpp
	pointmatcher/LoggerImpl.cpp
	pointmatcher/DataPointsFiltersImpl.cpp
	pointmatcher/MatchersImpl.cpp
	pointmatcher/OutlierFiltersImpl.cpp
	pointmatcher/ErrorMinimizersImpl.cpp
	pointmatcher/TransformationsImpl.cpp
	pointmatcher/TransformationCheckersImpl.cpp
	pointmatcher/InspectorsImpl.cpp
)

include_directories(${CMAKE_SOURCE_DIR})
file(GLOB POINTMATCHER_HEADERS "pointmatcher/*.h")

# Build library
add_library(pointmatcher SHARED ${POINTMATCHER_SRC} ${POINTMATCHER_HEADERS})
target_link_libraries(pointmatcher ${EXTERNAL_LIBS})
set_target_properties(pointmatcher PROPERTIES VERSION "${PROJECT_VERSION}" SOVERSION 1)
add_definitions(-Wall)

# Install
install(TARGETS pointmatcher
	ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
	LIBRARY DESTINATION ${INSTALL_LIB_DIR}
	RUNTIME DESTINATION ${INSTALL_BIN_DIR})

# On macOS, CMake's install step edits the library with install_name_tool,
# which invalidates its code signature; recent macOS then refuses to load it.
# Re-sign ad-hoc after install. No-op on Linux.
if (APPLE)
  install(CODE "
    file(GLOB signlibs \"\${CMAKE_INSTALL_PREFIX}/lib/libpointmatcher*.dylib\")
    foreach(f IN LISTS signlibs)
      execute_process(COMMAND codesign --force --sign - \${f} ERROR_QUIET)
    endforeach()
  ")
endif()

install(FILES
	pointmatcher/PointMatcher.h
	pointmatcher/PointMatcherPrivate.h
	pointmatcher/Parametrizable.h
	pointmatcher/Registrar.h
	pointmatcher/Timer.h
	pointmatcher/IO.h
	DESTINATION ${INSTALL_INCLUDE_DIR}/pointmatcher
)
install(FILES README.md DESTINATION share/doc/${PROJECT_NAME})
