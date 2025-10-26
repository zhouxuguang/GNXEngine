#ifndef GNX_ENGINE_PBUTILS_INCLUDE_GKDJGKJGKSGKDGK
#define GNX_ENGINE_PBUTILS_INCLUDE_GKDJGKJGKSGKDGK

#include "AssetProcessDefine.h"
#include "pb_common.h"
#include "pb_decode.h"
#include "pb_encode.h"

NS_ASSETPROCESS_BEGIN

bool nanopb_decode_gnx_bytes(pb_istream_t* stream, const pb_field_t* field, void** arg);

bool nanopb_encode_gnx_bytes(pb_ostream_t* stream, const pb_field_t* field, void* const* arg);

NS_ASSETPROCESS_END

#endif // !GNX_ENGINE_PBUTILS_INCLUDE_GKDJGKJGKSGKDGK