add_executable(ssr
    main.cpp
    SSRFrameWork.cpp
    SSRFrameWork.h)

target_link_libraries(ssr GNXEngine)

set_property(TARGET ssr PROPERTY FOLDER "demo/ssr")
