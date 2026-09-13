# ============================================================================
# copy_ios_assets.cmake — 把整个 data_asset 打包进 iOS bundle（资源根）
#
# 用法:
#   cmake -DSRC_DIR=<data_asset> -DDST_DIR=<bundle 根> -P copy_ios_assets.cmake
#
# 与 Android 端 assets.srcDirs += dataAssetDir 的语义一致：
#   data_asset/xxx → bundle 根/xxx（保留子目录结构）
# 运行时 AssetManager::LoadResource("Shader/xxx.msl_ios.gnxasset") /
# LoadResource("terrain/xx.png") 通过 SDL 解析到 bundle resourcePath 下的相对路径。
#
# 为控制包体积，仅复制移动端 iOS 实际需要的文件：
#   - 保留 *.msl_ios.gnxasset 合并容器 shader（剔除 .ps./.vs./.cs./.ts./.ms.
#     旧逐 stage 产物，以及 spirv / msl_macos 桌面格式）
#   - 非 shader 资源（png/jpg/ktx/obj 等）原样全量复制
# 目录结构（含空层级）与源保持一致。
# ============================================================================

if(NOT DEFINED SRC_DIR OR NOT DEFINED DST_DIR)
    message(FATAL_ERROR "copy_ios_assets.cmake: SRC_DIR and DST_DIR are required")
endif()

# 注意：绝不能 REMOVE_RECURSE "${DST_DIR}" —— 那是 bundle 根（.app 目录），
# 包含链接生成的可执行文件 (terrain)、Info.plist、embedded.mobileprovision、
# PkgInfo、_CodeSignature 等。POST_BUILD 在链接之后运行，删除 bundle 根会
# 连带删掉可执行文件导致构建失败。
#
# 关键冲突: data_asset 顶层可能与 bundle 保留文件同名（如 data_asset/terrain/
# 资源目录 vs bundle 根下的 terrain 可执行文件）。因此清理/复制时必须跳过
# "bundle 保留条目"（可执行文件 + bundle 元数据），避免把可执行文件覆盖成目录。
# 可执行文件名由 CMakeLists 通过 -DPRESERVE_NAMES 传入（分号分隔列表）。

# bundle 保留条目：可执行文件 + bundle 元数据（这些绝不能动）
set(BUNDLE_RESERVED
    Info.plist
    PkgInfo
    embedded.mobileprovision
    _CodeSignature
)
# 追加调用方传入的可执行文件名（如 terrain）
if(DEFINED PRESERVE_NAMES)
    foreach(n IN LISTS PRESERVE_NAMES)
        if(NOT n STREQUAL "")
            list(APPEND BUNDLE_RESERVED "${n}")
        endif()
    endforeach()
endif()

# 判断某顶层名字是否属于 bundle 保留条目
function(is_reserved name out_var)
    set(result FALSE)
    foreach(r IN LISTS BUNDLE_RESERVED)
        if(name STREQUAL r)
            set(result TRUE)
            break()
        endif()
    endforeach()
    set(${out_var} ${result} PARENT_SCOPE)
endfunction()

# 收集源目录顶层条目（子目录/文件），用于清理目标中已不存在的旧资源
file(GLOB TOP_ENTRIES "${SRC_DIR}/*")
foreach(entry IN LISTS TOP_ENTRIES)
    get_filename_component(name "${entry}" NAME)
    # 跳过 bundle 保留条目（如 data_asset/terrain/ 目录，不能动 bundle 的 terrain 可执行文件）
    is_reserved("${name}" reserved)
    if(reserved)
        continue()
    endif()
    set(dst_entry "${DST_DIR}/${name}")
    # 仅清理目标中属于 data_asset 的顶层资源（避免误删 bundle 其它内容）
    if(EXISTS "${dst_entry}")
        file(REMOVE_RECURSE "${dst_entry}")
    endif()
endforeach()

# 递归收集源目录下所有文件（相对路径）
file(GLOB_RECURSE ALL_FILES RELATIVE "${SRC_DIR}" "${SRC_DIR}/*")

set(copied 0)
set(skipped 0)
foreach(rel IN LISTS ALL_FILES)
    set(src "${SRC_DIR}/${rel}")

    # 只处理文件（跳过目录条目）
    if(NOT IS_DIRECTORY "${src}")
        # 相对路径首段若命中 bundle 保留条目（如 data_asset/terrain/ 下文件 → terrain/xxx），
        # 跳过 —— 否则会把 bundle 根的 terrain 可执行文件覆盖成目录。
        string(REPLACE "/" ";" rel_parts "${rel}")
        list(GET rel_parts 0 first_part)
        is_reserved("${first_part}" top_reserved)
        if(top_reserved)
            math(EXPR skipped "${skipped} + 1")
            continue()
        endif()

        set(should_copy TRUE)

        # 仅对 shader 产物（.gnxasset）做格式过滤：只保留 msl_ios 合并容器
        if(rel MATCHES "\\.gnxasset$")
            # 剔除旧逐 stage 产物（.ps. / .vs. / .cs. / .ts. / .ms. 前缀命名）
            if(rel MATCHES "\\.(ps|vs|cs|ts|ms)\\.(msl_ios|msl_macos|spirv|dxil|glsl)\\.gnxasset$")
                set(should_copy FALSE)
            # 剔除非 msl_ios 的合并容器（spirv / msl_macos 等桌面/Android 格式）
            elseif(NOT rel MATCHES "\\.msl_ios\\.gnxasset$")
                set(should_copy FALSE)
            endif()
        endif()

        if(should_copy)
            set(dst "${DST_DIR}/${rel}")
            get_filename_component(dst_dir "${dst}" DIRECTORY)
            file(MAKE_DIRECTORY "${dst_dir}")
            file(COPY "${src}" DESTINATION "${dst_dir}")
            math(EXPR copied "${copied} + 1")
        else()
            math(EXPR skipped "${skipped} + 1")
        endif()
    endif()
endforeach()

message(STATUS "copy_ios_assets: copied ${copied} files, skipped ${skipped} to ${DST_DIR}")
