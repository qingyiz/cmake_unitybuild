add_library(doctor_ui STATIC
    include/doctor/ui/analysis_controller.h
    include/doctor/ui/analysis_workspace_widget.h
    include/doctor/ui/main_window.h
    include/doctor/ui/project_setup_widget.h
    include/doctor/ui/ui_theme.h
    src/analysis_controller.cpp
    src/analysis_workspace_view.cpp
    src/analysis_workspace_widget.cpp
    src/main_window.cpp
    src/project_setup_view.cpp
    src/project_setup_widget.cpp
    src/ui_theme.cpp
)

target_include_directories(doctor_ui PUBLIC include)
target_link_libraries(doctor_ui
    PUBLIC doctor_application doctor_domain
    PRIVATE doctor_infrastructure "Qt${QT_VERSION_MAJOR}::Widgets"
)
