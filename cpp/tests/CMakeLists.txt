add_executable(doctor_smoke_tests
    smoke_test.cpp
)
target_link_libraries(
    doctor_smoke_tests
    PRIVATE doctor_domain doctor_ui
            "Qt${QT_VERSION_MAJOR}::Test"
            "Qt${QT_VERSION_MAJOR}::Widgets"
)

add_test(NAME doctor_smoke_tests COMMAND doctor_smoke_tests)
set_tests_properties(doctor_smoke_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(doctor_domain_tests domain_test.cpp)
target_link_libraries(
    doctor_domain_tests PRIVATE doctor_domain "Qt${QT_VERSION_MAJOR}::Test"
)
add_test(NAME doctor_domain_tests COMMAND doctor_domain_tests)

add_executable(doctor_backend_tests backend_test.cpp)
target_link_libraries(
    doctor_backend_tests
    PRIVATE doctor_infrastructure doctor_application doctor_domain
            "Qt${QT_VERSION_MAJOR}::Test"
)
target_compile_definitions(doctor_backend_tests PRIVATE
    TEST_CMAKE_COMMAND="${CMAKE_COMMAND}"
    TEST_CMAKE_GENERATOR="${CMAKE_GENERATOR}"
    TEST_FIXTURE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/multi_target"
)
add_test(NAME doctor_backend_tests COMMAND doctor_backend_tests)

add_executable(doctor_ui_tests ui_test.cpp)
target_link_libraries(
    doctor_ui_tests
    PRIVATE doctor_ui
            "Qt${QT_VERSION_MAJOR}::Test"
            "Qt${QT_VERSION_MAJOR}::Widgets"
)
target_compile_definitions(doctor_ui_tests PRIVATE
    TEST_CMAKE_COMMAND="${CMAKE_COMMAND}"
    TEST_CMAKE_GENERATOR="${CMAKE_GENERATOR}"
    TEST_FIXTURE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/multi_target"
)
add_test(NAME doctor_ui_tests COMMAND doctor_ui_tests)
set_tests_properties(doctor_ui_tests PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
