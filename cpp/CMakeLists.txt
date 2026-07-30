add_subdirectory(domain)
add_subdirectory(application)
add_subdirectory(infrastructure)
add_subdirectory(ui)
add_subdirectory(app)

if(UNITY_DOCTOR_BUILD_TESTS)
    add_subdirectory(tests)
endif()
