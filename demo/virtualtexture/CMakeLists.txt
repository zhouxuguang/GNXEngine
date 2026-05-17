add_executable(virtualtexture
    main.cpp
    VTFrameWork.cpp
    VTFrameWork.h
)

target_link_libraries(virtualtexture GNXEngine)

set_property(TARGET virtualtexture PROPERTY FOLDER "demo/virtualtexture")