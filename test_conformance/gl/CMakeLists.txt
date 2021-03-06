if(WIN32)
list(APPEND CLConform_LIBRARIES  glut32 opengl32 glu32 )
else(WIN32)
list(APPEND CLConform_LIBRARIES  GL glut GLEW GLU)
endif(WIN32)

set (GL_SOURCES
    main.cpp
    test_buffers.cpp
    test_images_2D.cpp
    test_images_3D.cpp
    test_renderbuffer.cpp
    test_images_2D_info.cpp
    test_images_3D_info.cpp
    test_renderbuffer_info.cpp
    test_fence_sync.cpp
    helpers.cpp
    ../../test_common/gl/helpers.cpp
    ../../test_common/harness/genericThread.cpp
    ../../test_common/harness/errorHelpers.c
    ../../test_common/harness/threadTesting.c
    ../../test_common/harness/testHarness.c
    ../../test_common/harness/kernelHelpers.c
    ../../test_common/harness/mt19937.c
    ../../test_common/harness/conversions.c
    ../../test_common/harness/msvc9.c
    ../../test_common/harness/parseParameters.cpp
    )

if (WIN32)
    list (APPEND GL_SOURCES ../../test_common/gl/setup_win32.cpp)
else(WIN32)
    list (APPEND GL_SOURCES ../../test_common/gl/setup_x11.cpp)
endif(WIN32)

# Compiling GLEW along with the project if the compiler is MINGW.
# The mingw linker was giving weird errors while linking to glew32.lib generated by
# MSVC.
if(MINGW)
    list (APPEND GL_SOURCES GLEW/glew.c)
    set_source_files_properties(
        ${GL_SOURCES}
        COMPILE_FLAGS -DGLEW_STATIC)
    include_directories("./GLEW/")
endif(MINGW)

if (MSVC)
    if(CMAKE_CL_64)
    list(APPEND CLConform_LIBRARIES glew64)
    else(CMAKE_CL_64)
    list(APPEND CLConform_LIBRARIES glew32)
    endif(CMAKE_CL_64)
endif(MSVC)

add_executable(conformance_test_gl
        ${GL_SOURCES} ${GLUT_SOURCES})

set_source_files_properties(
        ${GL_SOURCES}
        PROPERTIES LANGUAGE CXX)

TARGET_LINK_LIBRARIES(conformance_test_gl
        ${CLConform_LIBRARIES})
