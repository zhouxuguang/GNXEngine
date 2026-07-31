add_subdirectory(metis)

# Fix Android: metis detects execinfo.h/getline which don't exist on Android
if(ANDROID)
    target_compile_options(metis PRIVATE -UHAVE_EXECINFO_H -UHAVE_GETLINE)
endif()

set_property(TARGET metis PROPERTY FOLDER "ThirdParty")
