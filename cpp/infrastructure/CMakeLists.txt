add_library(doctor_infrastructure STATIC
    src/cmake_backend.cpp
    src/process_runner.cpp
    src/report_exporter.cpp
)

target_include_directories(doctor_infrastructure PUBLIC include)
target_link_libraries(
    doctor_infrastructure
    PUBLIC doctor_application doctor_domain "Qt${QT_VERSION_MAJOR}::Core"
)
