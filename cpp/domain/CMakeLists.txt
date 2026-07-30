add_library(doctor_domain STATIC
    src/diagnostics.cpp
    src/minimizer.cpp
    src/models.cpp
)

target_include_directories(doctor_domain PUBLIC include)
target_compile_features(doctor_domain PUBLIC cxx_std_17)
