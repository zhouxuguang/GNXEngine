
add_library(nanopb STATIC
            pb.h
            pb_common.h
            pb_common.c
            pb_encode.h
            pb_encode.c
            pb_decode.h
            pb_decode.c)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET nanopb PROPERTY FOLDER "thirdparty")