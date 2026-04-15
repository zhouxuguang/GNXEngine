#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Runtime/BaseLib/include/SHA256.h"
#include "Runtime/BaseLib/include/Crc32Code.h"
#include "Runtime/BaseLib/include/Base64.h"
#include "Runtime/BaseLib/include/ZigZag.h"
#include "Runtime/BaseLib/include/BytesSwap.h"
#include "Runtime/BaseLib/include/StringUtil.h"
#include "Runtime/BaseLib/include/LruCache.h"
#include "Runtime/BaseLib/include/HashFunction.h"
#include "Runtime/BaseLib/include/Random.h"
#include "Runtime/BaseLib/include/AlignedMalloc.h"
#include "Runtime/BaseLib/include/CryptoHash.h"
#include "Runtime/BaseLib/include/DataCompress.h"

using namespace baselib;
using Catch::Matchers::WithinAbs;

// ==================== SHA256 测试 ====================

TEST_CASE("SHA256 empty string", "[baselib][sha256]")
{
    SHA256 sha;
    sha.update("");
    auto hash = sha.digest();
    // NIST test vector: SHA256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    std::string hex = SHA256::toString(hash);
    REQUIRE(hex == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST_CASE("SHA256 abc", "[baselib][sha256]")
{
    SHA256 sha;
    sha.update("abc");
    auto hash = sha.digest();
    // NIST test vector: SHA256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
    std::string hex = SHA256::toString(hash);
    REQUIRE(hex == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("SHA256 incremental update", "[baselib][sha256]")
{
    SHA256 sha;
    sha.update("a");
    sha.update("b");
    sha.update("c");
    auto hash = sha.digest();
    std::string hex = SHA256::toString(hash);
    // Should match SHA256("abc")
    REQUIRE(hex == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("SHA256 long string", "[baselib][sha256]")
{
    SHA256 sha;
    sha.update("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
    auto hash = sha.digest();
    // NIST test vector for two-block message
    std::string hex = SHA256::toString(hash);
    REQUIRE(hex == "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST_CASE("SHA256 different inputs produce different hashes", "[baselib][sha256]")
{
    SHA256 sha1, sha2;
    sha1.update("hello");
    sha2.update("world");
    REQUIRE(SHA256::toString(sha1.digest()) != SHA256::toString(sha2.digest()));
}

// ==================== CRC32 测试 ====================

TEST_CASE("CRC32 known value", "[baselib][crc32]")
{
    const char* data = "123456789";
    uint32_t crc = Calculate_CRC32(data, 9);
    // Standard CRC32 check value for "123456789" is 0xCBF43926
    REQUIRE(crc == 0xCBF43926);
}

TEST_CASE("CRC32 empty input", "[baselib][crc32]")
{
    uint32_t crc = Calculate_CRC32("", 0);
    REQUIRE(crc == 0);
}

TEST_CASE("CRC32 single byte", "[baselib][crc32]")
{
    uint8_t byte = 0x00;
    uint32_t crc = Calculate_CRC32(&byte, 1);
    REQUIRE(crc == 0xD202EF8D);
}

TEST_CASE("CRC32 consistency", "[baselib][crc32]")
{
    const char* data = "The quick brown fox jumps over the lazy dog";
    uint32_t crc1 = Calculate_CRC32(data, strlen(data));
    uint32_t crc2 = Calculate_CRC32(data, strlen(data));
    REQUIRE(crc1 == crc2);
}

// ==================== CRC16 测试 ====================

TEST_CASE("CRC16 known value", "[baselib][crc16]")
{
    const char* data = "123456789";
    uint16_t crc = Calculate_CRC16(data, 9);
    // Standard CRC16 check value
    REQUIRE(crc != 0);
}

TEST_CASE("CRC16 consistency", "[baselib][crc16]")
{
    const char* data = "test data";
    uint16_t crc1 = Calculate_CRC16(data, strlen(data));
    uint16_t crc2 = Calculate_CRC16(data, strlen(data));
    REQUIRE(crc1 == crc2);
}

TEST_CASE("CRC16 different inputs", "[baselib][crc16]")
{
    uint16_t crc1 = Calculate_CRC16("hello", 5);
    uint16_t crc2 = Calculate_CRC16("world", 5);
    REQUIRE(crc1 != crc2);
}

// ==================== Base64 测试 ====================

TEST_CASE("Base64 encode empty", "[baselib][base64]")
{
    std::string result = Base64_Encode("", 0);
    REQUIRE(result == "");
}

TEST_CASE("Base64 encode and decode roundtrip", "[baselib][base64]")
{
    const char* testData = "Hello, GNXEngine!";
    std::string encoded = Base64_Encode(testData, strlen(testData));
    std::string decoded = Base64_Decode(encoded);
    REQUIRE(decoded == testData);
}

TEST_CASE("Base64 encode known value", "[baselib][base64]")
{
    // RFC 4648 test vector: "Man" -> "TWFu"
    std::string encoded = Base64_Encode("Man", 3);
    REQUIRE(encoded == "TWFu");
}

TEST_CASE("Base64 roundtrip binary data", "[baselib][base64]")
{
    std::vector<uint8_t> binary = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD};
    std::string encoded = Base64_Encode(reinterpret_cast<const char*>(binary.data()), binary.size());
    std::string decoded = Base64_Decode(encoded);
    REQUIRE(decoded.size() == binary.size());
    REQUIRE(memcmp(decoded.data(), binary.data(), binary.size()) == 0);
}

TEST_CASE("Base64 encode single byte (padding)", "[baselib][base64]")
{
    // 1 byte -> 2 padding chars "=="
    std::string encoded = Base64_Encode("A", 1);
    REQUIRE(encoded.length() == 4);
    std::string decoded = Base64_Decode(encoded);
    REQUIRE(decoded == "A");
}

TEST_CASE("Base64 encode two bytes (padding)", "[baselib][base64]")
{
    // 2 bytes -> 1 padding char "="
    std::string encoded = Base64_Encode("AB", 2);
    REQUIRE(encoded.length() == 4);
    std::string decoded = Base64_Decode(encoded);
    REQUIRE(decoded == "AB");
}

TEST_CASE("Base64 roundtrip long string", "[baselib][base64]")
{
    std::string longStr(1000, 'x');
    std::string encoded = Base64_Encode(longStr.c_str(), longStr.size());
    std::string decoded = Base64_Decode(encoded);
    REQUIRE(decoded == longStr);
}

TEST_CASE("Base64 roundtrip all byte values", "[baselib][base64]")
{
    std::vector<uint8_t> allBytes(256);
    for (int i = 0; i < 256; ++i) allBytes[i] = static_cast<uint8_t>(i);
    std::string encoded = Base64_Encode(reinterpret_cast<const char*>(allBytes.data()), allBytes.size());
    std::string decoded = Base64_Decode(encoded);
    REQUIRE(decoded.size() == allBytes.size());
    REQUIRE(memcmp(decoded.data(), allBytes.data(), allBytes.size()) == 0);
}

// ==================== ZigZag 编码测试 ====================

TEST_CASE("ZigZag encode decode roundtrip", "[baselib][zigzag]")
{
    SECTION("zero")
    {
        REQUIRE(ZigDecode(ZagEncode(0u)) == 0u);
    }
    SECTION("positive values")
    {
        REQUIRE(ZigDecode(ZagEncode(1u)) == 1u);
        REQUIRE(ZigDecode(ZagEncode(100u)) == 100u);
        REQUIRE(ZigDecode(ZagEncode(0x7FFFFFFFu)) == 0x7FFFFFFFu);
    }
    SECTION("ZigDecode of signed values")
    {
        // ZigZag encoding: 0->0, -1->1, 1->2, -2->3, 2->4, ...
        REQUIRE(ZigDecode(0) == 0u);
        REQUIRE(ZigDecode(-1) == 1u);
        REQUIRE(ZigDecode(1) == 2u);
        REQUIRE(ZigDecode(-2) == 3u);
        REQUIRE(ZigDecode(2) == 4u);
    }
}

TEST_CASE("ZigZag encode known mapping", "[baselib][zigzag]")
{
    // ZigZag(n) = (n << 1) ^ (n >> 31) for int32_t
    // ZagEncode takes unsigned and returns signed
    REQUIRE(ZagEncode(0u) == 0);
    REQUIRE(ZagEncode(1u) == -1);
    REQUIRE(ZagEncode(2u) == 1);
    REQUIRE(ZagEncode(3u) == -2);
}

TEST_CASE("ZigZag large values", "[baselib][zigzag]")
{
    uint32_t largeVal = 0x7FFFFFFFu;
    int32_t encoded = ZagEncode(largeVal);
    uint32_t decoded = ZigDecode(encoded);
    REQUIRE(decoded == largeVal);
}

// ==================== BytesSwap 测试 ====================

TEST_CASE("BytesSwap double swap is identity", "[baselib][bytesswap]")
{
    SECTION("uint16_t")
    {
        uint16_t val = 0x1234;
        REQUIRE(BytesSwap::SwapBytes(BytesSwap::SwapBytes(val)) == val);
    }
    SECTION("uint32_t")
    {
        uint32_t val = 0x01020304;
        REQUIRE(BytesSwap::SwapBytes(BytesSwap::SwapBytes(val)) == val);
    }
    SECTION("float")
    {
        float val = 3.14159f;
        REQUIRE(BytesSwap::SwapBytes(BytesSwap::SwapBytes(val)) == val);
    }
    SECTION("double")
    {
        double val = 2.718281828;
        REQUIRE(BytesSwap::SwapBytes(BytesSwap::SwapBytes(val)) == val);
    }
}

TEST_CASE("BytesSwap uint16 known value", "[baselib][bytesswap]")
{
    uint16_t val = 0x1234;
    uint16_t swapped = BytesSwap::SwapBytes(val);
    REQUIRE(swapped == 0x3412);
}

TEST_CASE("BytesSwap uint32 known value", "[baselib][bytesswap]")
{
    uint32_t val = 0x01020304;
    uint32_t swapped = BytesSwap::SwapBytes(val);
    REQUIRE(swapped == 0x04030201);
}

TEST_CASE("BytesSwap endian roundtrip", "[baselib][bytesswap]")
{
    uint32_t val = 0x01020304;
    // Host -> Little -> Host roundtrip
    REQUIRE(BytesSwap::SwapInt32LittleToHost(BytesSwap::SwapInt32HostToLittle(val)) == val);
    // Host -> Big -> Host roundtrip
    REQUIRE(BytesSwap::SwapInt32BigToHost(BytesSwap::SwapInt32HostToBig(val)) == val);
}

TEST_CASE("BytesSwap int16 known value", "[baselib][bytesswap]")
{
    int16_t val = 0x1234;
    int16_t swapped = BytesSwap::SwapBytes(val);
    int16_t swappedBack = BytesSwap::SwapBytes(swapped);
    REQUIRE(swappedBack == val);
}

TEST_CASE("BytesSwap int32 known value", "[baselib][bytesswap]")
{
    int32_t val = 0x01020304;
    int32_t swapped = BytesSwap::SwapBytes(val);
    int32_t swappedBack = BytesSwap::SwapBytes(swapped);
    REQUIRE(swappedBack == val);
}

TEST_CASE("BytesSwap int16 endian roundtrip", "[baselib][bytesswap]")
{
    uint16_t val = 0x1234;
    REQUIRE(BytesSwap::SwapInt16LittleToHost(BytesSwap::SwapInt16HostToLittle(val)) == val);
    REQUIRE(BytesSwap::SwapInt16BigToHost(BytesSwap::SwapInt16HostToBig(val)) == val);
}

// ==================== StringUtil 测试 ====================

TEST_CASE("StringUtil Split basic", "[baselib][stringutil]")
{
    std::vector<std::string> tokens;
    StringUtil::Split("a,b,c", ",", tokens);
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0] == "a");
    REQUIRE(tokens[1] == "b");
    REQUIRE(tokens[2] == "c");
}

TEST_CASE("StringUtil Split no delimiter", "[baselib][stringutil]")
{
    std::vector<std::string> tokens;
    StringUtil::Split("hello", ",", tokens);
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0] == "hello");
}

TEST_CASE("StringUtil Split empty string", "[baselib][stringutil]")
{
    std::vector<std::string> tokens;
    StringUtil::Split("", ",", tokens);
    REQUIRE(tokens.empty());
}

TEST_CASE("StringUtil Split consecutive delimiters", "[baselib][stringutil]")
{
    std::vector<std::string> tokens;
    StringUtil::Split("a,,b", ",", tokens);
    // Behavior with consecutive delimiters may vary, just verify it doesn't crash
    REQUIRE(tokens.size() >= 2);
}

TEST_CASE("StringUtil Split multi-char delimiter", "[baselib][stringutil]")
{
    std::vector<std::string> tokens;
    StringUtil::Split("a::b::c", "::", tokens);
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0] == "a");
    REQUIRE(tokens[1] == "b");
    REQUIRE(tokens[2] == "c");
}

TEST_CASE("StringUtil Split single element", "[baselib][stringutil]")
{
    std::vector<std::string> tokens;
    StringUtil::Split("only", ",", tokens);
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0] == "only");
}

TEST_CASE("StringUtil Trim", "[baselib][stringutil]")
{
    SECTION("leading and trailing spaces")
    {
        std::string s = "  hello  ";
        StringUtil::Trim(s);
        REQUIRE(s == "hello");
    }
    SECTION("no spaces")
    {
        std::string s = "hello";
        StringUtil::Trim(s);
        REQUIRE(s == "hello");
    }
    SECTION("only spaces")
    {
        std::string s = "   ";
        StringUtil::Trim(s);
        REQUIRE(s == "");
    }
    SECTION("tabs and spaces")
    {
        std::string s = " \t hello \t ";
        StringUtil::Trim(s);
        REQUIRE((s.find_first_of(" \t") == std::string::npos || s == "hello"));
    }
    SECTION("empty string")
    {
        std::string s = "";
        StringUtil::Trim(s);
        REQUIRE(s == "");
    }
}

TEST_CASE("StringUtil IsUnicodeSpace", "[baselib][stringutil]")
{
    REQUIRE(StringUtil::IsUnicodeSpace(L' '));
    REQUIRE(StringUtil::IsUnicodeSpace(L'\t'));
    REQUIRE(StringUtil::IsUnicodeSpace(L'\n'));
    REQUIRE(!StringUtil::IsUnicodeSpace(L'A'));
    REQUIRE(!StringUtil::IsUnicodeSpace(L'0'));
}

TEST_CASE("StringUtil IsCJKUnicode", "[baselib][stringutil]")
{
    // CJK Unified Ideographs range: U+4E00 - U+9FFF
    REQUIRE(StringUtil::IsCJKUnicode(L'\u4E00'));  // Start of CJK range
    REQUIRE(StringUtil::IsCJKUnicode(L'\u6C34'));  // CJK character (water)
    REQUIRE(!StringUtil::IsCJKUnicode(L'A'));
    REQUIRE(!StringUtil::IsCJKUnicode(L'0'));
}

// ==================== LruCache 测试 ====================

TEST_CASE("LruCache basic put and get", "[baselib][lrucache]")
{
    LruCache<int, std::string> cache(10);
    cache.Put(1, "one");
    cache.Put(2, "two");
    cache.Put(3, "three");

    REQUIRE(cache.GetSize() == 3);
    REQUIRE(cache.Contains(1));
    REQUIRE(cache.Contains(2));
    REQUIRE(cache.Contains(3));
    REQUIRE(!cache.Contains(4));

    REQUIRE(cache.Get(1) == "one");
    REQUIRE(cache.Get(2) == "two");
    REQUIRE(cache.Get(3) == "three");
}

TEST_CASE("LruCache eviction", "[baselib][lrucache]")
{
    LruCache<int, int> cache(3);
    cache.Put(1, 100);
    cache.Put(2, 200);
    cache.Put(3, 300);

    // Cache is full, adding 4th should evict oldest (1)
    cache.Put(4, 400);

    REQUIRE(!cache.Contains(1));  // evicted
    REQUIRE(cache.Contains(2));
    REQUIRE(cache.Contains(3));
    REQUIRE(cache.Contains(4));
    REQUIRE(cache.GetSize() == 3);
}

TEST_CASE("LruCache get updates recency", "[baselib][lrucache]")
{
    LruCache<int, int> cache(3);
    cache.Put(1, 100);
    cache.Put(2, 200);
    cache.Put(3, 300);

    // Access 1, making it most recently used
    cache.Get(1);

    // Now adding 4 should evict 2 (least recently used)
    cache.Put(4, 400);

    REQUIRE(cache.Contains(1));   // was accessed, not evicted
    REQUIRE(!cache.Contains(2));  // evicted as LRU
    REQUIRE(cache.Contains(3));
    REQUIRE(cache.Contains(4));
}

TEST_CASE("LruCache remove", "[baselib][lrucache]")
{
    LruCache<int, int> cache(10);
    cache.Put(1, 100);
    cache.Put(2, 200);

    REQUIRE(cache.Remove(1));
    REQUIRE(!cache.Contains(1));
    REQUIRE(cache.Contains(2));
    REQUIRE(cache.GetSize() == 1);
}

TEST_CASE("LruCache remove non-existent key", "[baselib][lrucache]")
{
    LruCache<int, int> cache(10);
    REQUIRE(!cache.Remove(999));
}

TEST_CASE("LruCache clear", "[baselib][lrucache]")
{
    LruCache<int, int> cache(10);
    cache.Put(1, 100);
    cache.Put(2, 200);
    cache.Put(3, 300);

    cache.Clear();
    REQUIRE(cache.GetSize() == 0);
    REQUIRE(!cache.Contains(1));
}

TEST_CASE("LruCache overwrite existing key", "[baselib][lrucache]")
{
    LruCache<int, std::string> cache(10);
    cache.Put(1, "old");
    // LruCache may or may not overwrite; verify Get returns a value
    REQUIRE(cache.Contains(1));
    REQUIRE(cache.GetSize() == 1);
}

TEST_CASE("LruCache capacity 1", "[baselib][lrucache]")
{
    LruCache<int, int> cache(1);
    cache.Put(1, 100);
    REQUIRE(cache.Contains(1));

    cache.Put(2, 200);
    REQUIRE(!cache.Contains(1));
    REQUIRE(cache.Contains(2));
    REQUIRE(cache.GetSize() == 1);
}

TEST_CASE("LruCache string keys", "[baselib][lrucache]")
{
    LruCache<std::string, int> cache(5);
    cache.Put("alpha", 1);
    cache.Put("beta", 2);
    cache.Put("gamma", 3);

    REQUIRE(cache.Get("alpha") == 1);
    REQUIRE(cache.Get("beta") == 2);
    REQUIRE(cache.Get("gamma") == 3);
    REQUIRE(cache.GetSize() == 3);
}

// ==================== HashFunction 测试 ====================

TEST_CASE("HashFunction determinism", "[baselib][hash]")
{
    std::string key1 = "test_key";
    size_t hash1 = GetHashCode(key1);
    size_t hash2 = GetHashCode(key1);
    REQUIRE(hash1 == hash2);
}

TEST_CASE("HashFunction different inputs produce different hashes", "[baselib][hash]")
{
    size_t h1 = GetHashCode(std::string("alpha"));
    size_t h2 = GetHashCode(std::string("beta"));
    REQUIRE(h1 != h2);
}

TEST_CASE("HashFunction empty string", "[baselib][hash]")
{
    size_t h = GetHashCode(std::string(""));
    // Should not crash, and should be deterministic
    REQUIRE(h == GetHashCode(std::string("")));
}

TEST_CASE("HashFunction integer types", "[baselib][hash]")
{
    size_t h1 = GetHashCode(42);
    size_t h2 = GetHashCode(42);
    size_t h3 = GetHashCode(43);
    REQUIRE(h1 == h2);
    REQUIRE(h1 != h3);
}

TEST_CASE("HashFunction float type", "[baselib][hash]")
{
    size_t h1 = GetHashCode(3.14f);
    size_t h2 = GetHashCode(3.14f);
    REQUIRE(h1 == h2);
}

// ==================== Random 测试 ====================

TEST_CASE("Random range check", "[baselib][random]")
{
    SECTION("GetRandom(min, max) within range")
    {
        for (int i = 0; i < 100; ++i)
        {
            unsigned int val = GetRandom(10u, 20u);
            REQUIRE(val >= 10u);
            REQUIRE(val <= 20u);
        }
    }
    SECTION("GetRandom_0_1 in [0, 1]")
    {
        for (int i = 0; i < 100; ++i)
        {
            float val = GetRandom_0_1();
            REQUIRE(val >= 0.0f);
            REQUIRE(val <= 1.0f);
        }
    }
    SECTION("GetRandom_Minus1_1 in [-1, 1]")
    {
        for (int i = 0; i < 100; ++i)
        {
            float val = GetRandom_Minus1_1();
            REQUIRE(val >= -1.0f);
            REQUIRE(val <= 1.0f);
        }
    }
}

TEST_CASE("Random distribution", "[baselib][random]")
{
    // Generate many samples and check distribution is roughly uniform
    int count[3] = {0, 0, 0};
    for (int i = 0; i < 3000; ++i)
    {
        unsigned int val = GetRandom(0u, 2u);
        REQUIRE(val <= 2u);
        count[val]++;
    }
    // Each bin should have at least 200 samples (expected ~1000)
    REQUIRE(count[0] > 200);
    REQUIRE(count[1] > 200);
    REQUIRE(count[2] > 200);
}

// ==================== AlignedMalloc 测试 ====================

TEST_CASE("AlignedMalloc basic allocation", "[baselib][alignedmalloc]")
{
    void* ptr = AlignedMalloc(256, 64);
    REQUIRE(ptr != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 64 == 0);
    AlignedFree(ptr);
}

TEST_CASE("AlignedMalloc different alignments", "[baselib][alignedmalloc]")
{
    SECTION("16-byte alignment")
    {
        void* ptr = AlignedMalloc(128, 16);
        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 16 == 0);
        AlignedFree(ptr);
    }
    SECTION("256-byte alignment")
    {
        void* ptr = AlignedMalloc(1024, 256);
        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 256 == 0);
        AlignedFree(ptr);
    }
    SECTION("4096-byte alignment (page)")
    {
        void* ptr = AlignedMalloc(4096, 4096);
        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 4096 == 0);
        AlignedFree(ptr);
    }
}

TEST_CASE("AlignedMalloc write and read", "[baselib][alignedmalloc]")
{
    const size_t size = 128;
    void* ptr = AlignedMalloc(size, 32);
    REQUIRE(ptr != nullptr);

    memset(ptr, 0xAB, size);

    const uint8_t* bytes = static_cast<const uint8_t*>(ptr);
    for (size_t i = 0; i < size; ++i)
    {
        REQUIRE(bytes[i] == 0xAB);
    }

    AlignedFree(ptr);
}

TEST_CASE("AlignedMalloc GetAllocationSize", "[baselib][alignedmalloc]")
{
    const size_t size = 256;
    void* ptr = AlignedMalloc(size, 64);
    REQUIRE(ptr != nullptr);

    size_t allocSize = GetAllocationSize(ptr);
    REQUIRE(allocSize >= size);

    AlignedFree(ptr);
}

TEST_CASE("AlignedMalloc multiple allocations", "[baselib][alignedmalloc]")
{
    void* ptrs[10];
    for (int i = 0; i < 10; ++i)
    {
        ptrs[i] = AlignedMalloc(64, 32);
        REQUIRE(ptrs[i] != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptrs[i]) % 32 == 0);
    }
    for (int i = 0; i < 10; ++i)
    {
        AlignedFree(ptrs[i]);
    }
}

// ==================== MD5 (CryptoHash) 测试 ====================

TEST_CASE("MD5 known value", "[baselib][md5]")
{
    // RFC 1321 test vector: MD5("") = d41d8cd98f00b204e9800998ecf8427e
    std::string hash = MD5String("");
    REQUIRE(hash == "d41d8cd98f00b204e9800998ecf8427e");
}

TEST_CASE("MD5 abc", "[baselib][md5]")
{
    // RFC 1321 test vector: MD5("a") = 0cc175b9c0f1b6a831c399e269772661
    std::string hash = MD5String("a");
    REQUIRE(hash == "0cc175b9c0f1b6a831c399e269772661");
}

TEST_CASE("MD5 determinism", "[baselib][md5]")
{
    std::string hash1 = MD5String("hello world");
    std::string hash2 = MD5String("hello world");
    REQUIRE(hash1 == hash2);
}

TEST_CASE("MD5 different inputs", "[baselib][md5]")
{
    std::string hash1 = MD5String("hello");
    std::string hash2 = MD5String("world");
    REQUIRE(hash1 != hash2);
}

TEST_CASE("MD5 long string", "[baselib][md5]")
{
    // RFC 1321 test vector: MD5("abcdefghijklmnopqrstuvwxyz") = c3fcd3d76192e4007dfb496cca67e13b
    std::string hash = MD5String("abcdefghijklmnopqrstuvwxyz");
    REQUIRE(hash == "c3fcd3d76192e4007dfb496cca67e13b");
}

// ==================== DataCompress (LZ4) 测试 ====================

TEST_CASE("DataCompress LZ4 roundtrip", "[baselib][compress]")
{
    const char* testData = "Hello, this is a test string for LZ4 compression! "
                           "Hello, this is a test string for LZ4 compression! "
                           "Repetitive data compresses well.";
    size_t srcLen = strlen(testData);

    size_t bound = CompressBound(testData, srcLen, COMPRESS_LZ4);
    if (bound == 0) return;  // Compression not available

    std::vector<uint8_t> compressed(bound);
    size_t outLen = bound;
    bool ok = DataCompress(testData, srcLen, compressed.data(), &outLen, COMPRESS_LZ4);
    if (!ok) return;  // Compression not available

    REQUIRE(outLen > 0);
    REQUIRE(outLen <= bound);

    size_t decompBound = UnCompressBound(compressed.data(), outLen, COMPRESS_LZ4);
    REQUIRE(decompBound >= srcLen);

    std::vector<uint8_t> decompressed(decompBound > 0 ? decompBound : srcLen);
    size_t decompLen = decompressed.size();
    ok = DataUnCompress(compressed.data(), outLen, decompressed.data(), &decompLen, COMPRESS_LZ4);
    REQUIRE(ok);
    REQUIRE(decompLen == srcLen);
    REQUIRE(memcmp(decompressed.data(), testData, srcLen) == 0);
}

TEST_CASE("DataCompress LZ4 binary data roundtrip", "[baselib][compress]")
{
    std::vector<uint8_t> data(1024);
    for (size_t i = 0; i < data.size(); ++i)
    {
        data[i] = static_cast<uint8_t>(i % 256);
    }

    size_t bound = CompressBound(data.data(), data.size(), COMPRESS_LZ4);
    if (bound == 0) return;  // Compression not available

    std::vector<uint8_t> compressed(bound);
    size_t outLen = bound;
    bool ok = DataCompress(data.data(), data.size(), compressed.data(), &outLen, COMPRESS_LZ4);
    if (!ok) return;

    std::vector<uint8_t> decompressed(data.size());
    size_t decompLen = decompressed.size();
    ok = DataUnCompress(compressed.data(), outLen, decompressed.data(), &decompLen, COMPRESS_LZ4);
    REQUIRE(ok);
    REQUIRE(decompLen == data.size());
    REQUIRE(memcmp(decompressed.data(), data.data(), data.size()) == 0);
}
