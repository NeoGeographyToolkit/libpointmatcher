cmake_minimum_required(VERSION 3.15)

include (CheckSymbolExists)

project(libpointmatcher)

# TODO(oalexan1): These need revisiting.
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE "Release")
add_definitions(-O3)

# Extract version from header
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
execute_process(
	COMMAND grep "POINTMATCHER_VERSION " pointmatcher/PointMatcher.h
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	RESULT_VARIABLE GREP_VERSION_RESULT
	OUTPUT_VARIABLE PROJECT_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (NOT GREP_VERSION_RESULT EQUAL 0)
	message(SEND_ERROR "Cannot grep version number: ${GREP_VERSION_RESULT}")
endif (NOT GREP_VERSION_RESULT EQUAL 0)
string(REGEX REPLACE ".*\"(.*)\".*" "\\1" PROJECT_VERSION "${PROJECT_VERSION}" )

# Check if 32 bit platform
# By default, libpointmatcher is not compatible with and will not build on a 
# 32 bit system
if( NOT CMAKE_SIZEOF_VOID_P EQUAL 8 ) 
    MESSAGE(SEND_ERROR "32 bits compiler detected. Libpointmatcher is only supported in 64 bits." )
    SET( EX_PLATFORM 32 )
    SET( EX_PLATFORM_NAME "x86" )
endif( NOT CMAKE_SIZEOF_VOID_P EQUAL 8 )

## WARNING: unsupported
## To force install as a 32 bit library, set BUILD_32 to true
if( BUILD_32 )
	MESSAGE(STATUS "Building as a 32 bit library")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif( BUILD_32 )

# For Windows
if( MSVC ) # VS2012 does not support tuples correctly yet
	add_definitions( /D _VARIADIC_MAX=10 )
endif()

#======================= installation =====================================

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH
  "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
	set(DEF_INSTALL_CMAKE_DIR CMake)
else()
	set(DEF_INSTALL_CMAKE_DIR lib/cmake/pointmatcher)
endif()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")


# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
	set(var INSTALL_${p}_DIR)
	if(NOT IS_ABSOLUTE "${${var}}")
		set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
	endif()
endforeach()

# Install package.xml for catkin
install(FILES package.xml DESTINATION "share/${PROJECT_NAME}")

#===========================================================================

# # Documentation
# set(DOXYFILE_LATEX false)
# include(UseDoxygen)

# initially
set(EXTERNAL_LIBS "")
set(EXTRA_DEPS "")

# For Boost with recent compilers must have this compiler switch
# to avoid link time failures
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_NO_CXX11_SCOPED_ENUMS")

# Boost
find_package(Boost COMPONENTS thread filesystem system program_options date_time REQUIRED)
if (Boost_MINOR_VERSION GREATER 47)
	find_package(Boost COMPONENTS thread filesystem system program_options date_time chrono REQUIRED)
endif (Boost_MINOR_VERSION GREATER 47)
include_directories(${Boost_INCLUDE_DIRS})
set(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${Boost_LIBRARIES})

# Eigen
find_package(Eigen3 3 REQUIRED NO_MODULE)
if (EIGEN3_FOUND)
   message("-- Found Eigen3: ${EIGEN3_INCLUDE_DIRS}")
else (EIGEN3_FOUND)
   message(FATAL_ERROR "Cannot find Eigen3.")
endif (EIGEN3_FOUND)
include_directories("${CMAKE_CURRENT_SOURCE_DIR};${EIGEN3_INCLUDE_DIR}")

#--------------------
# DEPENDENCY: nabo
#--------------------
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LIBNABO_INSTALL_DIR}/share/libnabo/cmake/")
#find_package(libnabo REQUIRED)
#include(libnaboConfig)
#include_directories(${libnabo_INCLUDE_DIRS})
set(EXTERNAL_LIBS ${EXTERNAL_LIBS} -lnabo)
#message("-- libnabo found, version ${libnabo_VERSION}")

# TODO(oalexan1): Figure out why clang does not find OpenMP
# Note: We use the path to $ENV{CONDA_PREFIX}
if (APPLE)
    # Use the active Conda environment path
    set(OPENMP_ROOT "$ENV{CONDA_PREFIX}")

    find_package(OpenMP)
    set(OPENMP_INCLUDES "${OPENMP_ROOT}/include")
    set(OpenMP_C_LIB_NAMES "omp")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_C_FLAGS "-fopenmp")
    set(OpenMP_CXX_FLAGS "-fopenmp")
    set(OpenMP_omp_LIBRARY "${OPENMP_ROOT}/lib/libomp${CMAKE_SHARED_LIBRARY_SUFFIX}")

    # Set this variable here so the global append at the bottom works
    set(OpenMP_EXE_LINKER_FLAGS "${OpenMP_omp_LIBRARY}")

    set(OpenMP_CXX_FOUND ON)
    set(WITH_OPENMP ON)
    message(STATUS "Manually set: OpenMP_CXX_FLAGS=${OpenMP_CXX_FLAGS}")
else()
    find_package(OpenMP REQUIRED)
endif()

# TODO(oalexan1): See if this works better.
# if(APPLE)
#     set(OpenMP_ROOT "$ENV{CONDA_PREFIX}")
# endif()
# find_package(OpenMP REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

#--------------------
# DEPENDENCY: OpenCL (optional)
#--------------------
set(USE_OPEN_CL "false" CACHE BOOL "Set to ON to look for OpenCL, if your libnabo was compiled with CL support")
if (USE_OPEN_CL)
	if (WIN32)
		find_library(OPENCL_LIBRARIES opencl64)
		if (!OPENCL_LIBRARIES)
			find_library(OPENCL_LIBRARIES opencl32)
		endif (!OPENCL_LIBRARIES)
	else (WIN32)
		find_library(OPENCL_LIBRARIES OpenCL ENV LD_LIBRARY_PATH)
	endif (WIN32)
	# if found, add
	if (OPENCL_LIBRARIES)
		set(EXTERNAL_LIBS ${EXTERNAL_LIBS} ${OPENCL_LIBRARIES})
    message("-- openCL found, parallel computer enabled for kd-tree")
	endif (OPENCL_LIBRARIES)
endif(USE_OPEN_CL)


check_symbol_exists(_POSIX_TIMERS "unistd.h;time.h" POSIX_TIMERS)
if (POSIX_TIMERS AND NOT APPLE)
	set(EXTERNAL_LIBS ${EXTERNAL_LIBS} rt)
endif (POSIX_TIMERS AND NOT APPLE)

# SOURCE

# Pointmatcher lib and install
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

set(SHARED_LIBS "true" CACHE BOOL "To build shared (true) or static (false) library")

if (SHARED_LIBS)
	add_library(pointmatcher SHARED ${POINTMATCHER_SRC} ${POINTMATCHER_HEADERS})
	install(TARGETS pointmatcher
					ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
					LIBRARY DESTINATION ${INSTALL_LIB_DIR}
					RUNTIME DESTINATION ${INSTALL_BIN_DIR})
							target_link_libraries(pointmatcher ${EXTERNAL_LIBS})
else(SHARED_LIBS)
	add_library(pointmatcher ${POINTMATCHER_SRC} ${POINTMATCHER_HEADERS} )
	install(TARGETS pointmatcher ARCHIVE DESTINATION ${INSTALL_LIB_DIR})
endif(SHARED_LIBS)

add_definitions(-Wall)
target_link_libraries(pointmatcher ${EXTERNAL_LIBS})

if(EXTRA_DEPS)
	add_dependencies(pointmatcher ${EXTRA_DEPS})
endif(EXTRA_DEPS)
set_target_properties(pointmatcher PROPERTIES VERSION "${PROJECT_VERSION}" SOVERSION 1)

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

