# Qt 6.4's FindWrapOpenGL unconditionally falls back to "-framework AGL" when
# the SDK no longer contains AGL. Modern Qt Widgets does not use AGL.
if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
    get_target_property(_doctor_wrap_opengl_links
        WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
    if(_doctor_wrap_opengl_links)
        list(FILTER _doctor_wrap_opengl_links EXCLUDE REGEX "AGL")
        set_property(TARGET WrapOpenGL::WrapOpenGL
            PROPERTY INTERFACE_LINK_LIBRARIES "${_doctor_wrap_opengl_links}")
    endif()
    unset(_doctor_wrap_opengl_links)
endif()
