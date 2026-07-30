cmake_minimum_required(VERSION 3.21)

project(UnityBuildDoctor VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(UNITY_DOCTOR_BUILD_TESTS "Build Unity Build Doctor tests" ON)
set(UNITY_DOCTOR_QT_MAJOR "" CACHE STRING "Qt major version used to build the app (5 or 6)")
set_property(CACHE UNITY_DOCTOR_QT_MAJOR PROPERTY STRINGS "" 5 6)
set(UNITY_DOCTOR_QT_ROOT "" CACHE PATH
    "Expected Qt installation prefix; presets use it to prevent fallback to another Qt kit")

if(UNITY_DOCTOR_QT_MAJOR AND
   NOT UNITY_DOCTOR_QT_MAJOR STREQUAL "5" AND
   NOT UNITY_DOCTOR_QT_MAJOR STREQUAL "6")
    message(FATAL_ERROR "UNITY_DOCTOR_QT_MAJOR must be 5 or 6")
endif()

if(UNITY_DOCTOR_QT_MAJOR)
    find_package(
        QT NAMES "Qt${UNITY_DOCTOR_QT_MAJOR}"
        REQUIRED COMPONENTS Core Widgets
    )
else()
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
endif()
find_package("Qt${QT_VERSION_MAJOR}" REQUIRED COMPONENTS Core Widgets)

if(UNITY_DOCTOR_QT_MAJOR AND
   NOT QT_VERSION_MAJOR STREQUAL UNITY_DOCTOR_QT_MAJOR)
    message(FATAL_ERROR
        "Requested Qt ${UNITY_DOCTOR_QT_MAJOR}, but found Qt ${QT_VERSION_MAJOR}")
endif()

if(UNITY_DOCTOR_QT_ROOT)
    if(NOT IS_DIRECTORY "${UNITY_DOCTOR_QT_ROOT}")
        message(FATAL_ERROR
            "UNITY_DOCTOR_QT_ROOT does not exist: ${UNITY_DOCTOR_QT_ROOT}")
    endif()

    set(qt_package_dir_variable "Qt${QT_VERSION_MAJOR}_DIR")
    set(qt_package_dir "${${qt_package_dir_variable}}")
    file(REAL_PATH "${UNITY_DOCTOR_QT_ROOT}" qt_root_real)
    file(REAL_PATH "${qt_package_dir}" qt_package_dir_real)
    cmake_path(IS_PREFIX qt_root_real "${qt_package_dir_real}" NORMALIZE
        qt_package_belongs_to_requested_root)
    if(NOT qt_package_belongs_to_requested_root)
        message(FATAL_ERROR
            "Requested Qt root ${qt_root_real}, but Qt ${QT_VERSION_MAJOR} "
            "was loaded from ${qt_package_dir_real}")
    endif()
endif()

if(QT_VERSION_MAJOR EQUAL 5 AND Qt5_VERSION VERSION_LESS 5.15)
    message(FATAL_ERROR "Unity Build Doctor requires Qt 5.15 or newer")
endif()
if(QT_VERSION_MAJOR EQUAL 6 AND Qt6_VERSION VERSION_LESS 6.4)
    message(FATAL_ERROR "Unity Build Doctor requires Qt 6.4 or newer")
endif()

message(STATUS "Unity Build Doctor Qt: ${QT_VERSION_MAJOR}")
include(cmake/QtMacOSCompatibility.cmake)
if(UNITY_DOCTOR_BUILD_TESTS)
    find_package("Qt${QT_VERSION_MAJOR}" REQUIRED COMPONENTS Test)
    include(CTest)
endif()

add_subdirectory(cpp)
