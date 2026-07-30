set(app_icon "${CMAKE_CURRENT_SOURCE_DIR}/resources/UnityBuildDoctor.icns")
set_source_files_properties(
    "${app_icon}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
)

add_executable(UnityBuildDoctor MACOSX_BUNDLE
    main.cpp
    "${app_icon}"
)

target_link_libraries(
    UnityBuildDoctor PRIVATE doctor_ui "Qt${QT_VERSION_MAJOR}::Widgets"
)

set_target_properties(UnityBuildDoctor PROPERTIES
    OUTPUT_NAME "UnityBuildDoctor"
    INSTALL_RPATH "@executable_path/../Frameworks"
    MACOSX_BUNDLE_ICON_FILE "UnityBuildDoctor.icns"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.unitybuilddoctor.app"
    MACOSX_BUNDLE_BUNDLE_NAME "Unity Build Doctor"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

install(TARGETS UnityBuildDoctor BUNDLE DESTINATION .)
