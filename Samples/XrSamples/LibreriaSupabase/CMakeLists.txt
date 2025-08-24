project(xrsamples_LibreriaSupabase)

# Es redundante pero sirve para encontrar errores en el debugging
if(NOT TARGET OpenXR::openxr_loader)
    find_package(OpenXR REQUIRED)
endif()

# NUEVO: Buscar archivos en la nueva estructura incluyendo Adapters
file(GLOB_RECURSE SRC_FILES
        Src/*.c
        Src/*.cpp
        Src/Telemetry/*.cpp
        Src/Telemetry/Adapters/*.cpp  # Incluir adapters
)

# En android se crea el libxrsamples_LibreriaSupabase.so y en windows el xrsamples_LibreriaSupabase.exe
if(ANDROID)
    add_library(${PROJECT_NAME} MODULE ${SRC_FILES})
    target_include_directories(${PROJECT_NAME} PUBLIC
            ${ANDROID_NDK}/sources/android/native_app_glue
            Src  # Para que encuentre los headers de Telemetry
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE
            android              # API Android nativa
            EGL                  # Context OpenGL ES
            GLESv3               # OpenGL ES 3.x
            log                  # Android logging (__android_log_print)
            ktx                  # Texturas KTX (compilada en 3rdParty)
    )
    # Android necesita -u ANativeActivity_onCreate como punto de entrada
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-u ANativeActivity_onCreate")

elseif(WIN32)
    add_definitions(-D_USE_MATH_DEFINES)
    add_executable(${PROJECT_NAME} ${SRC_FILES})

    # NUEVO: Incluir directorio de headers para Windows también
    target_include_directories(${PROJECT_NAME} PRIVATE Src)

    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_LIST_DIR}/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
            VERBATIM)

    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_SOURCE_DIR}/SampleXrFramework/res/raw"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/font/res/raw"
            VERBATIM)
endif()

# Acceso a todas las clases: OVRFW::XrApp, OVRFW::TinyUI, etc.
# Dependencias transitivas: Automáticamente enlaza con minizip, stb, ktx, OpenXR::openxr_loader
# Headers automáticos: Acceso a todos los includes del framework
target_include_directories(${PROJECT_NAME} PRIVATE Src)
target_link_libraries(${PROJECT_NAME} PRIVATE samplexrframework)