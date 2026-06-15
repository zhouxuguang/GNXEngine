add_executable(meshlet_gen
    main.cpp
)

target_link_libraries(meshlet_gen PRIVATE GNXEngine assimp meshoptimizer)

set_property(TARGET meshlet_gen PROPERTY FOLDER "tool/meshlet_gen")
