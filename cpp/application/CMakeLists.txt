add_library(doctor_application STATIC
    src/analysis_service.cpp
)

target_include_directories(doctor_application PUBLIC include)
target_link_libraries(
    doctor_application PUBLIC doctor_domain "Qt${QT_VERSION_MAJOR}::Core"
)
