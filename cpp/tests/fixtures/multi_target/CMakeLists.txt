cmake_minimum_required(VERSION 3.21)
project(UnityDoctorFixture LANGUAGES CXX)

add_library(good_target STATIC good.cpp)

add_library(baseline_bad STATIC baseline_bad.cpp)

add_library(unity_conflict STATIC conflict_a.cpp conflict_b.cpp)
set_target_properties(unity_conflict PROPERTIES UNITY_BUILD_BATCH_SIZE 20)
