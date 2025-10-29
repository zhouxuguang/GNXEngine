
set(mi_sources
    src/alloc.c
    src/alloc-aligned.c
    src/alloc-posix.c
    src/arena.c
    src/bitmap.c
    src/heap.c
    src/init.c
    src/libc.c
    src/options.c
    src/os.c
    src/page.c
    src/random.c
    src/segment.c
    src/segment-map.c
    src/stats.c
    src/prim/prim.c)


# single object file for more predictable static overriding
add_library(mimalloc STATIC src/static.c)
set_property(TARGET mimalloc PROPERTY POSITION_INDEPENDENT_CODE ON)
#target_compile_definitions(mimalloc PRIVATE ${mi_defines})
#target_compile_options(mimalloc PRIVATE ${mi_cflags} ${mi_cflags_static})
target_include_directories(mimalloc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_property(TARGET mimalloc PROPERTY FOLDER "ThirdParty")
