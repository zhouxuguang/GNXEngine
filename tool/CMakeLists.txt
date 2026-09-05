add_subdirectory(meshlet_gen)

# shader_compile is a host-only tool (macOS/Windows); skip on Android/iOS
if(NOT ANDROID AND NOT IOS)
    add_subdirectory(shader_compile)
    # pbr_asset_baker: offline PBR asset baking (KTX textures, IBL cubemaps,
    # meshasset). Host-only, uses GPU-independent AssetProcess/AssetManager.
    add_subdirectory(pbr_asset_baker)
endif()
