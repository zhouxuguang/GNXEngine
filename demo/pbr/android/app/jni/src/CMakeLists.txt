set(GNX_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../)

add_library(pbr SHARED
    main.cpp
    ${GNX_ROOT}/demo/pbr/PBRFrameWork.cpp
    # ImageTextureUtil references the CPU BRDF LUT fallback while AssetProcess is omitted.
    ${GNX_ROOT}/Engine/Runtime/AssetProcess/source/IBLBaker/PBRBase.cpp
    ${GNX_ROOT}/Engine/Runtime/AssetProcess/source/TextureProcess/stb_image_resize2.c
)

target_include_directories(pbr PRIVATE
    ${GNX_ROOT}/Engine
    ${GNX_ROOT}/Engine/Runtime/AssetProcess/include
    ${GNX_ROOT}/Engine/Runtime/AssetProcess/source
    ${GNX_ROOT}/Engine/Runtime/AssetProcess/source/IBLBaker
    ${GNX_ROOT}/ThirdParty/SDL2/SDL/include
    ${GNX_ROOT}/demo/pbr
)

target_link_libraries(pbr GNXEngine AssetManager nanopb ktx SDL2-static android log)
set_target_properties(pbr PROPERTIES DEBUG_POSTFIX "" OUTPUT_NAME "pbr")
target_link_options(pbr PRIVATE
    "-Wl,-z,max-page-size=16384"
    "-Wl,-z,common-page-size=16384"
)
