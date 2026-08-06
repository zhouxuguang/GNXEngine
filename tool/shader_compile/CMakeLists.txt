add_executable(shader_compile
    main.cpp
)

target_link_libraries(shader_compile PRIVATE GNXEngine nanopb)

set_property(TARGET shader_compile PROPERTY FOLDER "Tool")
