include_guard(GLOBAL)

function(unity_doctor_deploy_macos_bundle target_name)
    if(NOT APPLE)
        return()
    endif()

    if(NOT TARGET "${target_name}")
        message(FATAL_ERROR
            "Cannot deploy unknown macOS bundle target: ${target_name}")
    endif()

    set(macdeployqt_candidates)
    if(UNITY_DOCTOR_QT_ROOT)
        list(APPEND macdeployqt_candidates
            "${UNITY_DOCTOR_QT_ROOT}/bin/macdeployqt")
    endif()

    set(qt_package_dir_variable "Qt${QT_VERSION_MAJOR}_DIR")
    set(qt_package_dir "${${qt_package_dir_variable}}")
    if(qt_package_dir)
        get_filename_component(qt_cmake_dir "${qt_package_dir}" DIRECTORY)
        get_filename_component(qt_lib_dir "${qt_cmake_dir}" DIRECTORY)
        get_filename_component(qt_prefix "${qt_lib_dir}" DIRECTORY)
        list(APPEND macdeployqt_candidates "${qt_prefix}/bin/macdeployqt")
    endif()

    if(MACDEPLOYQT_EXECUTABLE)
        list(APPEND macdeployqt_candidates "${MACDEPLOYQT_EXECUTABLE}")
    endif()

    set(macdeployqt_executable)
    foreach(candidate IN LISTS macdeployqt_candidates)
        if(EXISTS "${candidate}")
            set(macdeployqt_executable "${candidate}")
            break()
        endif()
    endforeach()

    if(NOT macdeployqt_executable)
        message(FATAL_ERROR
            "macdeployqt for Qt ${QT_VERSION_MAJOR} was not found. "
            "Set UNITY_DOCTOR_QT_ROOT to the selected Qt kit prefix.")
    endif()

    message(STATUS
        "Unity Build Doctor build-tree deployment: ${macdeployqt_executable}")

    add_custom_command(
        TARGET "${target_name}" POST_BUILD
        COMMAND "${macdeployqt_executable}"
            "$<TARGET_BUNDLE_DIR:${target_name}>"
            -always-overwrite
        COMMAND /usr/bin/codesign
            --force
            --deep
            --sign -
            "$<TARGET_BUNDLE_DIR:${target_name}>"
        COMMAND /usr/bin/codesign
            --verify
            --deep
            --strict
            "$<TARGET_BUNDLE_DIR:${target_name}>"
        COMMENT
            "Deploying Qt runtime into ${target_name}.app and applying ad-hoc signature"
        VERBATIM
    )
endfunction()
