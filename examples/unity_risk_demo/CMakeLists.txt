cmake_minimum_required(VERSION 3.16)

project(UnityRiskDemo LANGUAGES CXX)

add_executable(unity_risk_demo
    main.cpp
    macro_a.cpp
    macro_b.cpp
    safe_patterns.cpp
    static_a.cpp
    static_b.cpp
    using_namespace.cpp
)

target_compile_features(unity_risk_demo PRIVATE cxx_std_17)

# 确保演示源码进入同一个 Unity batch，方便构建验证稳定复现 static 重定义。
set_target_properties(unity_risk_demo PROPERTIES UNITY_BUILD_BATCH_SIZE 32)
