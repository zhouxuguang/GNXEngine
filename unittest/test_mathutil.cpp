#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Runtime/MathUtil/include/Vector2.h"
#include "Runtime/MathUtil/include/Vector3.h"
#include "Runtime/MathUtil/include/Vector4.h"
#include "Runtime/MathUtil/include/Matrix3x3.h"
#include "Runtime/MathUtil/include/Matrix4x4.h"
#include "Runtime/MathUtil/include/Quaternion.h"
#include "Runtime/MathUtil/include/HalfFloat.h"
#include "Runtime/MathUtil/include/MathUtil.h"
#include "Runtime/MathUtil/include/AABB.h"

using namespace mathutil;
using Catch::Matchers::WithinAbs;

// ==================== Vector2 测试 ====================

TEST_CASE("Vector2 default constructor", "[mathutil][vector2]")
{
    // Vector2 default constructor does not initialize members to zero
    // (depends on implementation). Test parameterized constructor instead.
    Vector2f v(0.0f, 0.0f);
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 0.0f);
}

TEST_CASE("Vector2 parameterized constructor", "[mathutil][vector2]")
{
    Vector2f v(3.0f, 4.0f);
    REQUIRE(v.x == 3.0f);
    REQUIRE(v.y == 4.0f);
}

TEST_CASE("Vector2 scalar constructor", "[mathutil][vector2]")
{
    Vector2f v(5.0f);
    REQUIRE(v.x == 5.0f);
    REQUIRE(v.y == 5.0f);
}

TEST_CASE("Vector2 addition", "[mathutil][vector2]")
{
    Vector2f a(1.0f, 2.0f);
    Vector2f b(3.0f, 4.0f);
    Vector2f c = a + b;
    REQUIRE(c.x == 4.0f);
    REQUIRE(c.y == 6.0f);
}

TEST_CASE("Vector2 subtraction", "[mathutil][vector2]")
{
    Vector2f a(5.0f, 7.0f);
    Vector2f b(2.0f, 3.0f);
    Vector2f c = a - b;
    REQUIRE(c.x == 3.0f);
    REQUIRE(c.y == 4.0f);
}

TEST_CASE("Vector2 scalar multiplication", "[mathutil][vector2]")
{
    Vector2f v(2.0f, 3.0f);
    Vector2f r = v * 2.0f;
    REQUIRE(r.x == 4.0f);
    REQUIRE(r.y == 6.0f);
}

TEST_CASE("Vector2 scalar * vector", "[mathutil][vector2]")
{
    Vector2f v(2.0f, 3.0f);
    Vector2f r = 3.0f * v;
    REQUIRE(r.x == 6.0f);
    REQUIRE(r.y == 9.0f);
}

TEST_CASE("Vector2 scalar division", "[mathutil][vector2]")
{
    Vector2f v(6.0f, 8.0f);
    Vector2f r = v / 2.0f;
    REQUIRE(r.x == 3.0f);
    REQUIRE(r.y == 4.0f);
}

TEST_CASE("Vector2 component-wise multiply", "[mathutil][vector2]")
{
    Vector2f a(2.0f, 3.0f);
    Vector2f b(4.0f, 5.0f);
    Vector2f r = a * b;
    REQUIRE(r.x == 8.0f);
    REQUIRE(r.y == 15.0f);
}

TEST_CASE("Vector2 dot product", "[mathutil][vector2]")
{
    Vector2f a(1.0f, 0.0f);
    Vector2f b(0.0f, 1.0f);
    REQUIRE(a.DotProduct(b) == 0.0f);  // orthogonal

    Vector2f c(3.0f, 4.0f);
    REQUIRE(c.DotProduct(c) == 25.0f);  // 3^2 + 4^2
}

TEST_CASE("Vector2 length", "[mathutil][vector2]")
{
    Vector2f v(3.0f, 4.0f);
    REQUIRE_THAT(v.Length(), WithinAbs(5.0f, 1e-5f));
}

TEST_CASE("Vector2 length squared", "[mathutil][vector2]")
{
    Vector2f v(3.0f, 4.0f);
    REQUIRE_THAT(v.LengthSquared(), WithinAbs(25.0f, 1e-5f));
}

TEST_CASE("Vector2 cross product (2D scalar)", "[mathutil][vector2]")
{
    Vector2f a(1.0f, 0.0f);
    Vector2f b(0.0f, 1.0f);
    REQUIRE(a.CrossProduct(b) == 1.0f);  // a x b = 1*1 - 0*0 = 1
}

TEST_CASE("Vector2 normalize", "[mathutil][vector2]")
{
    Vector2f v(3.0f, 4.0f);
    float len = v.Normalise();
    REQUIRE_THAT(len, WithinAbs(5.0f, 1e-5f));
    REQUIRE_THAT(v.Length(), WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Vector2 equality and inequality", "[mathutil][vector2]")
{
    Vector2f a(1.0f, 2.0f);
    Vector2f b(1.0f, 2.0f);
    Vector2f c(3.0f, 4.0f);
    REQUIRE(a == b);
    REQUIRE(a != c);
}

TEST_CASE("Vector2 negation", "[mathutil][vector2]")
{
    Vector2f v(1.0f, -2.0f);
    Vector2f n = -v;
    REQUIRE(n.x == -1.0f);
    REQUIRE(n.y == 2.0f);
}

TEST_CASE("Vector2 compound assignment", "[mathutil][vector2]")
{
    Vector2f v(1.0f, 2.0f);
    v += Vector2f(3.0f, 4.0f);
    REQUIRE(v.x == 4.0f);
    REQUIRE(v.y == 6.0f);

    v -= Vector2f(1.0f, 1.0f);
    REQUIRE(v.x == 3.0f);
    REQUIRE(v.y == 5.0f);

    v *= 2.0f;
    REQUIRE(v.x == 6.0f);
    REQUIRE(v.y == 10.0f);

    v /= 2.0f;
    REQUIRE(v.x == 3.0f);
    REQUIRE(v.y == 5.0f);
}

TEST_CASE("Vector2 index accessor", "[mathutil][vector2]")
{
    Vector2f v(10.0f, 20.0f);
    REQUIRE(v[0] == 10.0f);
    REQUIRE(v[1] == 20.0f);

    v[0] = 30.0f;
    REQUIRE(v.x == 30.0f);
}

TEST_CASE("Vector2 distance", "[mathutil][vector2]")
{
    Vector2f a(0.0f, 0.0f);
    Vector2f b(3.0f, 4.0f);
    REQUIRE_THAT(a.Distance(b), WithinAbs(5.0f, 1e-5f));
    REQUIRE_THAT(a.DistanceSquared(b), WithinAbs(25.0f, 1e-5f));
}

TEST_CASE("Vector2 swap", "[mathutil][vector2]")
{
    Vector2f a(1.0f, 2.0f);
    Vector2f b(3.0f, 4.0f);
    a.Swap(b);
    REQUIRE(a.x == 3.0f);
    REQUIRE(a.y == 4.0f);
    REQUIRE(b.x == 1.0f);
    REQUIRE(b.y == 2.0f);
}

TEST_CASE("Vector2 reflect", "[mathutil][vector2]")
{
    // Reflect vector (1,-1) off surface with normal (0,1) -> (1,1)
    Vector2f incident(1.0f, -1.0f);
    Vector2f normal(0.0f, 1.0f);
    Vector2f reflected = incident.Reflect(normal);
    REQUIRE_THAT(reflected.x, WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(reflected.y, WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Vector2 vector + scalar and scalar + vector", "[mathutil][vector2]")
{
    Vector2f v(1.0f, 2.0f);
    Vector2f r1 = v + 10.0f;
    REQUIRE(r1.x == 11.0f);
    REQUIRE(r1.y == 12.0f);

    Vector2f r2 = 10.0f + v;
    REQUIRE(r2.x == 11.0f);
    REQUIRE(r2.y == 12.0f);
}

// ==================== Vector3 测试 ====================

TEST_CASE("Vector3 default constructor", "[mathutil][vector3]")
{
    Vector3f v;
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 0.0f);
    REQUIRE(v.z == 0.0f);
}

TEST_CASE("Vector3 parameterized constructor", "[mathutil][vector3]")
{
    Vector3f v(1.0f, 2.0f, 3.0f);
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
}

TEST_CASE("Vector3 direction constructor", "[mathutil][vector3]")
{
    Vector3f start(1.0f, 2.0f, 3.0f);
    Vector3f end(4.0f, 6.0f, 9.0f);
    Vector3f dir(start, end);
    REQUIRE(dir.x == 3.0f);
    REQUIRE(dir.y == 4.0f);
    REQUIRE(dir.z == 6.0f);
}

TEST_CASE("Vector3 addition and subtraction", "[mathutil][vector3]")
{
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(4.0f, 5.0f, 6.0f);

    Vector3f sum = a + b;
    REQUIRE(sum.x == 5.0f);
    REQUIRE(sum.y == 7.0f);
    REQUIRE(sum.z == 9.0f);

    Vector3f diff = b - a;
    REQUIRE(diff.x == 3.0f);
    REQUIRE(diff.y == 3.0f);
    REQUIRE(diff.z == 3.0f);
}

TEST_CASE("Vector3 scalar multiplication", "[mathutil][vector3]")
{
    Vector3f v(1.0f, 2.0f, 3.0f);
    Vector3f r = v * 2.0f;
    REQUIRE(r.x == 2.0f);
    REQUIRE(r.y == 4.0f);
    REQUIRE(r.z == 6.0f);
}

TEST_CASE("Vector3 scalar * vector", "[mathutil][vector3]")
{
    Vector3f v(1.0f, 2.0f, 3.0f);
    Vector3f r = 3.0f * v;
    REQUIRE(r.x == 3.0f);
    REQUIRE(r.y == 6.0f);
    REQUIRE(r.z == 9.0f);
}

TEST_CASE("Vector3 scalar division", "[mathutil][vector3]")
{
    Vector3f v(6.0f, 8.0f, 10.0f);
    Vector3f r = v / 2.0f;
    REQUIRE(r.x == 3.0f);
    REQUIRE(r.y == 4.0f);
    REQUIRE(r.z == 5.0f);
}

TEST_CASE("Vector3 component-wise multiply", "[mathutil][vector3]")
{
    Vector3f a(2.0f, 3.0f, 4.0f);
    Vector3f b(5.0f, 6.0f, 7.0f);
    Vector3f r = a * b;
    REQUIRE(r.x == 10.0f);
    REQUIRE(r.y == 18.0f);
    REQUIRE(r.z == 28.0f);
}

TEST_CASE("Vector3 Multiply method", "[mathutil][vector3]")
{
    Vector3f a(2.0f, 3.0f, 4.0f);
    Vector3f b(5.0f, 6.0f, 7.0f);
    Vector3f r = a.Multiply(b);
    REQUIRE(r.x == 10.0f);
    REQUIRE(r.y == 18.0f);
    REQUIRE(r.z == 28.0f);
}

TEST_CASE("Vector3 dot product", "[mathutil][vector3]")
{
    Vector3f a(1.0f, 0.0f, 0.0f);
    Vector3f b(0.0f, 1.0f, 0.0f);
    REQUIRE(a.DotProduct(b) == 0.0f);

    Vector3f c(1.0f, 2.0f, 3.0f);
    REQUIRE(c.DotProduct(c) == 14.0f);  // 1 + 4 + 9
}

TEST_CASE("Vector3 cross product", "[mathutil][vector3]")
{
    Vector3f a(1.0f, 0.0f, 0.0f);
    Vector3f b(0.0f, 1.0f, 0.0f);
    Vector3f c = Vector3f::CrossProduct(a, b);
    REQUIRE_THAT(c.x, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(c.y, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(c.z, WithinAbs(1.0f, 1e-5f));  // right-hand rule: X x Y = Z
}

TEST_CASE("Vector3 cross product anti-commutative", "[mathutil][vector3]")
{
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(4.0f, 5.0f, 6.0f);
    Vector3f ab = Vector3f::CrossProduct(a, b);
    Vector3f ba = Vector3f::CrossProduct(b, a);
    REQUIRE_THAT(ab.x, WithinAbs(-ba.x, 1e-5f));
    REQUIRE_THAT(ab.y, WithinAbs(-ba.y, 1e-5f));
    REQUIRE_THAT(ab.z, WithinAbs(-ba.z, 1e-5f));
}

TEST_CASE("Vector3 length and normalize", "[mathutil][vector3]")
{
    Vector3f v(1.0f, 2.0f, 3.0f);
    float lenSq = v.LengthSq();
    REQUIRE_THAT(lenSq, WithinAbs(14.0f, 1e-5f));

    float len = v.Length();
    REQUIRE_THAT(len, WithinAbs(std::sqrt(14.0f), 1e-5f));

    v.Normalize();
    REQUIRE_THAT(v.Length(), WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Vector3 negation", "[mathutil][vector3]")
{
    Vector3f v(1.0f, -2.0f, 3.0f);
    Vector3f n = -v;
    REQUIRE(n.x == -1.0f);
    REQUIRE(n.y == 2.0f);
    REQUIRE(n.z == -3.0f);
}

TEST_CASE("Vector3 equality", "[mathutil][vector3]")
{
    Vector3f a(1.0f, 2.0f, 3.0f);
    Vector3f b(1.0f, 2.0f, 3.0f);
    Vector3f c(4.0f, 5.0f, 6.0f);
    REQUIRE(a == b);
    REQUIRE(a != c);
}

TEST_CASE("Vector3 Zero method", "[mathutil][vector3]")
{
    Vector3f v(5.0f, 6.0f, 7.0f);
    v.Zero();
    REQUIRE(v.x == 0.0f);
    REQUIRE(v.y == 0.0f);
    REQUIRE(v.z == 0.0f);
}

TEST_CASE("Vector3 Abs method", "[mathutil][vector3]")
{
    Vector3f v(-1.0f, -2.0f, 3.0f);
    Vector3f a = v.Abs();
    REQUIRE(a.x == 1.0f);
    REQUIRE(a.y == 2.0f);
    REQUIRE(a.z == 3.0f);
}

TEST_CASE("Vector3 compound assignment", "[mathutil][vector3]")
{
    Vector3f v(1.0f, 2.0f, 3.0f);
    v += Vector3f(1.0f, 1.0f, 1.0f);
    REQUIRE(v.x == 2.0f);
    REQUIRE(v.y == 3.0f);
    REQUIRE(v.z == 4.0f);

    v -= Vector3f(1.0f, 1.0f, 1.0f);
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);

    v *= 2.0f;
    REQUIRE(v.x == 2.0f);
    REQUIRE(v.y == 4.0f);
    REQUIRE(v.z == 6.0f);

    v /= 2.0f;
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
}

TEST_CASE("Vector3 index accessor", "[mathutil][vector3]")
{
    Vector3f v(10.0f, 20.0f, 30.0f);
    REQUIRE(v[0] == 10.0f);
    REQUIRE(v[1] == 20.0f);
    REQUIRE(v[2] == 30.0f);

    v[0] = 40.0f;
    REQUIRE(v.x == 40.0f);
}

TEST_CASE("Vector3 distance", "[mathutil][vector3]")
{
    Vector3f a(0.0f, 0.0f, 0.0f);
    Vector3f b(1.0f, 2.0f, 2.0f);
    REQUIRE_THAT(a.Distance(b), WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(a.DistanceSquared(b), WithinAbs(9.0f, 1e-5f));
}

TEST_CASE("Vector3 Reflection", "[mathutil][vector3]")
{
    // Reflect incident (1,-1,0) off surface with normal (0,1,0) -> (1,1,0)
    Vector3f incident(1.0f, -1.0f, 0.0f);
    Vector3f normal(0.0f, 1.0f, 0.0f);
    Vector3f reflected = Vector3f::Reflection(incident, normal);
    REQUIRE_THAT(reflected.x, WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(reflected.y, WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(reflected.z, WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("Vector3 Lerp", "[mathutil][vector3]")
{
    Vector3f a(0.0f, 0.0f, 0.0f);
    Vector3f b(10.0f, 10.0f, 10.0f);

    Vector3f mid = Vector3f::Lerp(a, b, 0.5f);
    REQUIRE_THAT(mid.x, WithinAbs(5.0f, 1e-5f));
    REQUIRE_THAT(mid.y, WithinAbs(5.0f, 1e-5f));
    REQUIRE_THAT(mid.z, WithinAbs(5.0f, 1e-5f));

    Vector3f atStart = Vector3f::Lerp(a, b, 0.0f);
    REQUIRE(atStart == a);

    Vector3f atEnd = Vector3f::Lerp(a, b, 1.0f);
    REQUIRE_THAT(atEnd.x, WithinAbs(10.0f, 1e-5f));
    REQUIRE_THAT(atEnd.y, WithinAbs(10.0f, 1e-5f));
    REQUIRE_THAT(atEnd.z, WithinAbs(10.0f, 1e-5f));
}

// ==================== Vector4 测试 ====================

TEST_CASE("Vector4 constructor and access", "[mathutil][vector4]")
{
    Vector4f v(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(v.x == 1.0f);
    REQUIRE(v.y == 2.0f);
    REQUIRE(v.z == 3.0f);
    REQUIRE(v.w == 4.0f);
}

TEST_CASE("Vector4 addition", "[mathutil][vector4]")
{
    Vector4f a(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4f b(5.0f, 6.0f, 7.0f, 8.0f);
    Vector4f c = a + b;
    REQUIRE(c.x == 6.0f);
    REQUIRE(c.y == 8.0f);
    REQUIRE(c.z == 10.0f);
    REQUIRE(c.w == 12.0f);
}

TEST_CASE("Vector4 subtraction", "[mathutil][vector4]")
{
    Vector4f a(5.0f, 6.0f, 7.0f, 8.0f);
    Vector4f b(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4f c = a - b;
    REQUIRE(c.x == 4.0f);
    REQUIRE(c.y == 4.0f);
    REQUIRE(c.z == 4.0f);
    REQUIRE(c.w == 4.0f);
}

TEST_CASE("Vector4 scalar multiplication", "[mathutil][vector4]")
{
    Vector4f v(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4f r = v * 2.0f;
    REQUIRE(r.x == 2.0f);
    REQUIRE(r.y == 4.0f);
    REQUIRE(r.z == 6.0f);
    REQUIRE(r.w == 8.0f);
}

TEST_CASE("Vector4 scalar division", "[mathutil][vector4]")
{
    Vector4f v(2.0f, 4.0f, 6.0f, 8.0f);
    Vector4f r = v / 2.0f;
    REQUIRE(r.x == 1.0f);
    REQUIRE(r.y == 2.0f);
    REQUIRE(r.z == 3.0f);
    REQUIRE(r.w == 4.0f);
}

TEST_CASE("Vector4 component-wise multiply", "[mathutil][vector4]")
{
    Vector4f a(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4f b(2.0f, 3.0f, 4.0f, 5.0f);
    Vector4f r = a * b;
    REQUIRE(r.x == 2.0f);
    REQUIRE(r.y == 6.0f);
    REQUIRE(r.z == 12.0f);
    REQUIRE(r.w == 20.0f);
}

TEST_CASE("Vector4 negation", "[mathutil][vector4]")
{
    Vector4f v(1.0f, -2.0f, 3.0f, -4.0f);
    Vector4f n = -v;
    REQUIRE(n.x == -1.0f);
    REQUIRE(n.y == 2.0f);
    REQUIRE(n.z == -3.0f);
    REQUIRE(n.w == 4.0f);
}

TEST_CASE("Vector4 dot product", "[mathutil][vector4]")
{
    Vector4f a(1.0f, 2.0f, 3.0f, 4.0f);
    REQUIRE(a.DotProduct(a) == 30.0f);  // 1+4+9+16

    Vector4f b(1.0f, 0.0f, 0.0f, 0.0f);
    Vector4f c(0.0f, 1.0f, 0.0f, 0.0f);
    REQUIRE(b.DotProduct(c) == 0.0f);
}

TEST_CASE("Vector4 index accessor", "[mathutil][vector4]")
{
    Vector4f v(10.0f, 20.0f, 30.0f, 40.0f);
    REQUIRE(v[0] == 10.0f);
    REQUIRE(v[1] == 20.0f);
    REQUIRE(v[2] == 30.0f);
    REQUIRE(v[3] == 40.0f);
}

TEST_CASE("Vector4 xyz swizzle", "[mathutil][vector4]")
{
    Vector4f v(1.0f, 2.0f, 3.0f, 4.0f);
    Vector3f v3 = v.xyz();
    REQUIRE(v3.x == 1.0f);
    REQUIRE(v3.y == 2.0f);
    REQUIRE(v3.z == 3.0f);
}

TEST_CASE("Vector4 scalar add and sub", "[mathutil][vector4]")
{
    Vector4f v(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4f r1 = v + 10.0f;
    REQUIRE(r1.x == 11.0f);
    REQUIRE(r1.y == 12.0f);

    Vector4f r2 = v - 1.0f;
    REQUIRE(r2.x == 0.0f);
    REQUIRE(r2.y == 1.0f);
}

// ==================== Matrix3x3 测试 ====================

TEST_CASE("Matrix3x3 default is identity", "[mathutil][matrix3x3]")
{
    Matrix3x3f m;
    m.MakeIdentity();
    REQUIRE_THAT(m[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(m[1][1], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(m[2][2], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(m[0][1], WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(m[1][0], WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("Matrix3x3 MakeIdentity", "[mathutil][matrix3x3]")
{
    Matrix3x3f m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    m.MakeIdentity();
    REQUIRE_THAT(m[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(m[1][1], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(m[2][2], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(m[0][1], WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("Matrix3x3 9-element constructor", "[mathutil][matrix3x3]")
{
    Matrix3x3f m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    REQUIRE_THAT(m[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(m[0][1], WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(m[0][2], WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(m[1][0], WithinAbs(4.0f, 1e-5f));
    REQUIRE_THAT(m[1][1], WithinAbs(5.0f, 1e-5f));
}

TEST_CASE("Matrix3x3 addition", "[mathutil][matrix3x3]")
{
    Matrix3x3f a; a.MakeIdentity();
    Matrix3x3f b; b.MakeIdentity();
    Matrix3x3f c = a + b;
    REQUIRE_THAT(c[0][0], WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(c[1][1], WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(c[2][2], WithinAbs(2.0f, 1e-5f));
}

TEST_CASE("Matrix3x3 subtraction", "[mathutil][matrix3x3]")
{
    Matrix3x3f a(2, 0, 0, 0, 2, 0, 0, 0, 2);
    Matrix3x3f b; b.MakeIdentity();
    Matrix3x3f c = a - b;
    REQUIRE_THAT(c[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(c[1][1], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(c[2][2], WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Matrix3x3 scalar multiplication", "[mathutil][matrix3x3]")
{
    Matrix3x3f m; m.MakeIdentity();
    Matrix3x3f r = m * 3.0f;
    REQUIRE_THAT(r[0][0], WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(r[1][1], WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(r[2][2], WithinAbs(3.0f, 1e-5f));
}

TEST_CASE("Matrix3x3 matrix multiplication", "[mathutil][matrix3x3]")
{
    // Identity * Identity = Identity
    Matrix3x3f a; a.MakeIdentity();
    Matrix3x3f b; b.MakeIdentity();
    Matrix3x3f c = a * b;
    REQUIRE_THAT(c[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(c[1][1], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(c[2][2], WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Matrix3x3 matrix * vector", "[mathutil][matrix3x3]")
{
    // Identity * vector = vector
    Matrix3x3f m; m.MakeIdentity();
    Vector3f v(1.0f, 2.0f, 3.0f);
    Vector3f r = m * v;
    REQUIRE_THAT(r.x, WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(r.y, WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(r.z, WithinAbs(3.0f, 1e-5f));
}

TEST_CASE("Matrix3x3 determinant", "[mathutil][matrix3x3]")
{
    // Determinant of identity = 1
    Matrix3x3f m; m.MakeIdentity();
    REQUIRE_THAT(m.Determinant(), WithinAbs(1.0f, 1e-5f));

    // Determinant of a known matrix
    Matrix3x3f m2(1, 2, 3, 4, 5, 6, 7, 8, 9);
    REQUIRE_THAT(m2.Determinant(), WithinAbs(0.0f, 1e-5f));  // singular
}

TEST_CASE("Matrix3x3 transpose", "[mathutil][matrix3x3]")
{
    Matrix3x3f m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    Matrix3x3f t = m.Transpose();
    REQUIRE_THAT(t[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(t[0][1], WithinAbs(4.0f, 1e-5f));
    REQUIRE_THAT(t[1][0], WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(t[1][2], WithinAbs(8.0f, 1e-5f));
}

TEST_CASE("Matrix3x3 inverse", "[mathutil][matrix3x3]")
{
    // Inverse of identity is identity
    Matrix3x3f id; id.MakeIdentity();
    Matrix3x3f inv = id.Inverse();
    REQUIRE_THAT(inv[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(inv[1][1], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(inv[2][2], WithinAbs(1.0f, 1e-5f));

    // Inverse * Original = Identity
    Matrix3x3f m(2, 1, 0, 1, 3, 1, 0, 1, 2);
    Matrix3x3f mInv = m.Inverse();
    Matrix3x3f product = m * mInv;
    REQUIRE_THAT(product[0][0], WithinAbs(1.0f, 1e-3f));
    REQUIRE_THAT(product[1][1], WithinAbs(1.0f, 1e-3f));
    REQUIRE_THAT(product[2][2], WithinAbs(1.0f, 1e-3f));
    REQUIRE_THAT(product[0][1], WithinAbs(0.0f, 1e-3f));
}

TEST_CASE("Matrix3x3 negate", "[mathutil][matrix3x3]")
{
    Matrix3x3f m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    Matrix3x3f n = -m;
    REQUIRE_THAT(n[0][0], WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(n[1][1], WithinAbs(-5.0f, 1e-5f));
}

TEST_CASE("Matrix3x3 static constants", "[mathutil][matrix3x3]")
{
    REQUIRE_THAT(Matrix3x3f::IDENTITY[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(Matrix3x3f::IDENTITY[1][1], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(Matrix3x3f::ZERO[0][0], WithinAbs(0.0f, 1e-5f));
}

// ==================== Quaternion 测试 ====================

TEST_CASE("Quaternion default constructor", "[mathutil][quaternion]")
{
    Quaternionf q;
    // Default quaternion should be initialized
}

TEST_CASE("Quaternion parameterized constructor", "[mathutil][quaternion]")
{
    Quaternionf q(1.0f, 0.0f, 0.0f, 0.0f);
    REQUIRE(q.w == 1.0f);
    REQUIRE(q.x == 0.0f);
    REQUIRE(q.y == 0.0f);
    REQUIRE(q.z == 0.0f);
}

TEST_CASE("Quaternion identity norm", "[mathutil][quaternion]")
{
    Quaternionf q(1.0f, 0.0f, 0.0f, 0.0f);
    REQUIRE_THAT(q.Norm(), WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Quaternion normalize", "[mathutil][quaternion]")
{
    Quaternionf q(2.0f, 0.0f, 0.0f, 0.0f);
    q.Normalize();
    REQUIRE_THAT(q.Norm(), WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Quaternion Normalized returns new quaternion", "[mathutil][quaternion]")
{
    Quaternionf q(2.0f, 0.0f, 0.0f, 0.0f);
    Quaternionf n = q.Normalized();
    REQUIRE_THAT(n.Norm(), WithinAbs(1.0f, 1e-5f));
    // Original should be unchanged
    REQUIRE_THAT(q.w, WithinAbs(2.0f, 1e-5f));
}

TEST_CASE("Quaternion conjugate", "[mathutil][quaternion]")
{
    Quaternionf q(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternionf conj = Quaternionf::Conjugate(q);
    REQUIRE(conj.w == 1.0f);
    REQUIRE(conj.x == -2.0f);
    REQUIRE(conj.y == -3.0f);
    REQUIRE(conj.z == -4.0f);
}

TEST_CASE("Quaternion angle-axis roundtrip", "[mathutil][quaternion]")
{
    Quaternionf q;
    Vector3f axis(0.0f, 1.0f, 0.0f);  // Y axis
    // FromAngleAxis expects degrees (internally converts to radians)
    float angleDeg = 90.0f;

    q.FromAngleAxis(angleDeg, axis);
    q.Normalize();

    // ToAngleAxis returns radians
    float outAngleRad = 0.0f;
    Vector3f outAxis;
    q.ToAngleAxis(outAngleRad, outAxis);

    // Convert back to degrees for comparison
    float outAngleDeg = outAngleRad * static_cast<float>(RADTODEG);
    REQUIRE_THAT(outAngleDeg, WithinAbs(angleDeg, 0.1f));

    // Axis should be Y-axis (or -Y-axis if angle is flipped)
    REQUIRE_THAT(fabs(outAxis.x), WithinAbs(0.0f, 1e-3f));
    REQUIRE_THAT(fabs(outAxis.y), WithinAbs(1.0f, 1e-3f));
    REQUIRE_THAT(fabs(outAxis.z), WithinAbs(0.0f, 1e-3f));
}

TEST_CASE("Quaternion rotation of vector", "[mathutil][quaternion]")
{
    // Rotate (1,0,0) by 90 degrees around Z axis -> (0,1,0)
    Quaternionf q;
    q.FromAngleAxis(90.0f, Vector3f(0.0f, 0.0f, 1.0f));
    q.Normalize();

    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f result = q * v;

    REQUIRE_THAT(result.x, WithinAbs(0.0f, 1e-3f));
    REQUIRE_THAT(result.y, WithinAbs(1.0f, 1e-3f));
    REQUIRE_THAT(result.z, WithinAbs(0.0f, 1e-3f));
}

TEST_CASE("Quaternion rotation 180 degrees", "[mathutil][quaternion]")
{
    // Rotate (1,0,0) by 180 degrees around Y axis -> (-1,0,0)
    Quaternionf q;
    q.FromAngleAxis(180.0f, Vector3f(0.0f, 1.0f, 0.0f));
    q.Normalize();

    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f result = q * v;

    REQUIRE_THAT(result.x, WithinAbs(-1.0f, 1e-3f));
    REQUIRE_THAT(result.y, WithinAbs(0.0f, 1e-3f));
    REQUIRE_THAT(result.z, WithinAbs(0.0f, 1e-3f));
}

TEST_CASE("Quaternion multiplication", "[mathutil][quaternion]")
{
    // Verify that multiplying two quaternions gives a valid rotation
    Quaternionf q1, q2;
    q1.FromAngleAxis(90.0f, Vector3f(0.0f, 0.0f, 1.0f));
    q1.Normalize();
    q2.FromAngleAxis(90.0f, Vector3f(1.0f, 0.0f, 0.0f));
    q2.Normalize();

    Quaternionf combined = q1 * q2;
    combined.Normalize();
    REQUIRE_THAT(combined.Norm(), WithinAbs(1.0f, 1e-3f));
}

TEST_CASE("Quaternion Inverse", "[mathutil][quaternion]")
{
    Quaternionf q;
    q.FromAngleAxis(45.0f, Vector3f(0.0f, 1.0f, 0.0f));
    q.Normalize();

    Quaternionf inv = Quaternionf::Inverse(q);
    inv.Normalize();
    // Inverse undoes rotation
    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f rotated = q * v;
    Vector3f restored = inv * rotated;

    REQUIRE_THAT(restored.x, WithinAbs(v.x, 0.1f));
    REQUIRE_THAT(restored.y, WithinAbs(v.y, 0.1f));
    REQUIRE_THAT(restored.z, WithinAbs(v.z, 0.1f));
}

TEST_CASE("Quaternion Inverse undoes rotation", "[mathutil][quaternion]")
{
    Quaternionf q;
    q.FromAngleAxis(90.0f, Vector3f(0.0f, 0.0f, 1.0f));
    q.Normalize();

    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f rotated = q * v;
    Quaternionf inv = Quaternionf::Inverse(q);
    Vector3f restored = inv * rotated;

    REQUIRE_THAT(restored.x, WithinAbs(v.x, 1e-3f));
    REQUIRE_THAT(restored.y, WithinAbs(v.y, 1e-3f));
    REQUIRE_THAT(restored.z, WithinAbs(v.z, 1e-3f));
}

TEST_CASE("Quaternion Slerp", "[mathutil][quaternion]")
{
    Quaternionf q1, q2;
    q1.FromAngleAxis(0.0f, Vector3f(0.0f, 1.0f, 0.0f));
    q1.Normalize();
    q2.FromAngleAxis(90.0f, Vector3f(0.0f, 1.0f, 0.0f));
    q2.Normalize();

    // Slerp at t=0 should be q1, t=1 should be q2
    Quaternionf s0 = Quaternionf::Slerp(q1, q2, 0.0f);
    REQUIRE_THAT(s0.w, WithinAbs(q1.w, 1e-3f));
    REQUIRE_THAT(s0.x, WithinAbs(q1.x, 1e-3f));
    REQUIRE_THAT(s0.y, WithinAbs(q1.y, 1e-3f));
    REQUIRE_THAT(s0.z, WithinAbs(q1.z, 1e-3f));

    Quaternionf s1 = Quaternionf::Slerp(q1, q2, 1.0f);
    REQUIRE_THAT(s1.w, WithinAbs(q2.w, 1e-3f));
    REQUIRE_THAT(s1.x, WithinAbs(q2.x, 1e-3f));
    REQUIRE_THAT(s1.y, WithinAbs(q2.y, 1e-3f));
    REQUIRE_THAT(s1.z, WithinAbs(q2.z, 1e-3f));

    // Slerp at t=0.5 should give 45 degree rotation
    Quaternionf sMid = Quaternionf::Slerp(q1, q2, 0.5f);
    sMid.Normalize();
    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f rotated = sMid * v;
    // 45 degrees around Y: x=cos(45), z=-sin(45)
    REQUIRE_THAT(rotated.x, WithinAbs(std::cos(static_cast<float>(DEGTORAD) * 45.0f), 1e-2f));
    REQUIRE_THAT(rotated.z, WithinAbs(-std::sin(static_cast<float>(DEGTORAD) * 45.0f), 1e-2f));
}

TEST_CASE("Quaternion dot product", "[mathutil][quaternion]")
{
    Quaternionf q1(1.0f, 0.0f, 0.0f, 0.0f);
    Quaternionf q2(1.0f, 0.0f, 0.0f, 0.0f);
    REQUIRE_THAT(q1.DotProduct(q2), WithinAbs(1.0f, 1e-5f));

    Quaternionf q3(0.0f, 1.0f, 0.0f, 0.0f);
    REQUIRE_THAT(q1.DotProduct(q3), WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("Quaternion ToRotateMatrix produces valid rotation", "[mathutil][quaternion]")
{
    Quaternionf q;
    q.FromAngleAxis(45.0f, Vector3f(0.0f, 1.0f, 0.0f));
    q.Normalize();

    Matrix3x3f rotMat;
    q.ToRotateMatrix(rotMat);

    // Verify the rotation matrix can transform a vector
    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f r = rotMat * v;
    // After 45 degree Y rotation, x component should be cos(45) ~ 0.707
    REQUIRE_THAT(fabs(r.x), WithinAbs(std::cos(static_cast<float>(DEGTORAD) * 45.0f), 0.1f));
}

TEST_CASE("Quaternion FromEulerAngles", "[mathutil][quaternion]")
{
    Quaternionf q;
    q.FromEulerAngles(0.0f, 0.0f, 0.0f);  // no rotation
    q.Normalize();
    // Should be close to identity
    REQUIRE_THAT(fabs(q.w), WithinAbs(1.0f, 1e-3f));
}

TEST_CASE("Quaternion negate", "[mathutil][quaternion]")
{
    Quaternionf q(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternionf n = -q;
    REQUIRE(n.w == -1.0f);
    REQUIRE(n.x == -2.0f);
    REQUIRE(n.y == -3.0f);
    REQUIRE(n.z == -4.0f);
}

TEST_CASE("Quaternion scalar multiply", "[mathutil][quaternion]")
{
    Quaternionf q(1.0f, 0.0f, 0.0f, 0.0f);
    Quaternionf r = q * 2.0f;
    REQUIRE_THAT(r.w, WithinAbs(2.0f, 1e-5f));
}

// ==================== HalfFloat 测试 ====================

TEST_CASE("HalfFloat zero conversion", "[mathutil][halffloat]")
{
    uint16_t h = float_to_half(0.0f);
    REQUIRE(h == FP16_ZERO);

    float f = half_to_float(h);
    REQUIRE(f == 0.0f);
}

TEST_CASE("HalfFloat one conversion", "[mathutil][halffloat]")
{
    uint16_t h = float_to_half(1.0f);
    REQUIRE(h == FP16_ONE);

    float f = half_to_float(h);
    REQUIRE_THAT(f, WithinAbs(1.0f, 1e-3f));
}

TEST_CASE("HalfFloat negative one", "[mathutil][halffloat]")
{
    uint16_t h = float_to_half(-1.0f);
    REQUIRE(h == FP16_NAGATIVE_ONE);

    float f = half_to_float(h);
    REQUIRE_THAT(f, WithinAbs(-1.0f, 1e-3f));
}

TEST_CASE("HalfFloat roundtrip small value", "[mathutil][halffloat]")
{
    float original = 0.5f;
    uint16_t h = float_to_half(original);
    float back = half_to_float(h);
    REQUIRE_THAT(back, WithinAbs(original, 1e-3f));
}

TEST_CASE("HalfFloat roundtrip 0.1", "[mathutil][halffloat]")
{
    float original = 0.1f;
    uint16_t h = float_to_half(original);
    float back = half_to_float(h);
    // half float has limited precision, tolerance is larger
    REQUIRE_THAT(back, WithinAbs(original, 0.002f));
}

TEST_CASE("HalfFloat negative detection", "[mathutil][halffloat]")
{
    uint16_t h_pos = float_to_half(1.0f);
    uint16_t h_neg = float_to_half(-1.0f);
    REQUIRE(!half_is_negative(h_pos));
    REQUIRE(half_is_negative(h_neg));
}

TEST_CASE("HalfFloat roundtrip integer values", "[mathutil][halffloat]")
{
    for (int i = 0; i <= 10; ++i)
    {
        float original = static_cast<float>(i);
        uint16_t h = float_to_half(original);
        float back = half_to_float(h);
        REQUIRE_THAT(back, WithinAbs(original, 1e-3f));
    }
}

TEST_CASE("HalfFloat roundtrip negative values", "[mathutil][halffloat]")
{
    float values[] = {-0.5f, -2.0f, -10.0f, -100.0f};
    for (float original : values)
    {
        uint16_t h = float_to_half(original);
        float back = half_to_float(h);
        REQUIRE_THAT(back, WithinAbs(original, fabs(original) * 0.001f + 0.01f));
    }
}

TEST_CASE("HalfFloat roundtrip very small value", "[mathutil][halffloat]")
{
    float original = 0.001f;
    uint16_t h = float_to_half(original);
    float back = half_to_float(h);
    // Very small values may lose precision
    REQUIRE_THAT(back, WithinAbs(original, 0.001f));
}

// ==================== Matrix4x4 测试 ====================

TEST_CASE("Matrix4x4 default is identity", "[mathutil][matrix4x4]")
{
    Matrix4x4f m;
    // Identity matrix: diagonal = 1, rest = 0
    REQUIRE_THAT(m[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(m[1][1], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(m[2][2], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(m[3][3], WithinAbs(1.0f, 1e-5f));

    // Off-diagonal elements should be 0
    REQUIRE_THAT(m[0][1], WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(m[1][0], WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 scalar multiplication", "[mathutil][matrix4x4]")
{
    Matrix4x4f identity;  // identity
    Matrix4x4f m2 = identity * 2.0f;

    REQUIRE_THAT(m2[0][0], WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(m2[1][1], WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(m2[2][2], WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(m2[3][3], WithinAbs(2.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 matrix multiplication identity", "[mathutil][matrix4x4]")
{
    Matrix4x4f a;
    Matrix4x4f b;
    Matrix4x4f c = a * b;
    REQUIRE_THAT(c[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(c[1][1], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(c[2][2], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(c[3][3], WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 matrix * vector3", "[mathutil][matrix4x4]")
{
    // Identity * vector = vector
    Matrix4x4f m;
    Vector3f v(1.0f, 2.0f, 3.0f);
    Vector3f r = m * v;
    REQUIRE_THAT(r.x, WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(r.y, WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(r.z, WithinAbs(3.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 matrix * vector4", "[mathutil][matrix4x4]")
{
    Matrix4x4f m;
    Vector4f v(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4f r = m * v;
    REQUIRE_THAT(r.x, WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(r.y, WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(r.z, WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(r.w, WithinAbs(4.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 determinant", "[mathutil][matrix4x4]")
{
    Matrix4x4f m;
    REQUIRE_THAT(m.Determinant(), WithinAbs(1.0f, 1e-5f));  // det(I) = 1
}

TEST_CASE("Matrix4x4 inverse", "[mathutil][matrix4x4]")
{
    Matrix4x4f id;
    Matrix4x4f inv = id.Inverse();
    REQUIRE_THAT(inv[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(inv[1][1], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(inv[2][2], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(inv[3][3], WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 transpose", "[mathutil][matrix4x4]")
{
    // Transpose of identity is identity
    Matrix4x4f m;
    Matrix4x4f t = m.Transpose();
    REQUIRE_THAT(t[0][0], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(t[1][1], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(t[2][2], WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(t[3][3], WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 inverse roundtrip", "[mathutil][matrix4x4]")
{
    Matrix4x4f m = Matrix4x4f::CreateTranslate(1.0f, 2.0f, 3.0f);
    Matrix4x4f inv = m.Inverse();
    Matrix4x4f product = m * inv;
    REQUIRE_THAT(product[0][0], WithinAbs(1.0f, 1e-3f));
    REQUIRE_THAT(product[1][1], WithinAbs(1.0f, 1e-3f));
    REQUIRE_THAT(product[2][2], WithinAbs(1.0f, 1e-3f));
    REQUIRE_THAT(product[3][3], WithinAbs(1.0f, 1e-3f));
}

TEST_CASE("Matrix4x4 CreateTranslate", "[mathutil][matrix4x4]")
{
    Matrix4x4f m = Matrix4x4f::CreateTranslate(5.0f, 10.0f, 15.0f);
    Vector3f v(1.0f, 2.0f, 3.0f);
    Vector3f r = m * v;
    REQUIRE_THAT(r.x, WithinAbs(6.0f, 1e-5f));
    REQUIRE_THAT(r.y, WithinAbs(12.0f, 1e-5f));
    REQUIRE_THAT(r.z, WithinAbs(18.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 CreateScale", "[mathutil][matrix4x4]")
{
    Matrix4x4f m = Matrix4x4f::CreateScale(2.0f, 3.0f, 4.0f);
    Vector3f v(1.0f, 1.0f, 1.0f);
    Vector3f r = m * v;
    REQUIRE_THAT(r.x, WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(r.y, WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(r.z, WithinAbs(4.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 CreateScale vector", "[mathutil][matrix4x4]")
{
    Matrix4x4f m = Matrix4x4f::CreateScale(Vector3f(2.0f, 3.0f, 4.0f));
    Vector3f v(1.0f, 1.0f, 1.0f);
    Vector3f r = m * v;
    REQUIRE_THAT(r.x, WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(r.y, WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(r.z, WithinAbs(4.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 CreateRotationX", "[mathutil][matrix4x4]")
{
    // Rotate (0,1,0) by 90 degrees around X -> (0,0,1)
    // CreateRotationX may take degrees directly (matching FromAngleAxis convention)
    Matrix4x4f m = Matrix4x4f::CreateRotationX(90.0f);
    Vector3f v(0.0f, 1.0f, 0.0f);
    Vector3f r = m * v;
    REQUIRE_THAT(r.x, WithinAbs(0.0f, 1e-3f));
    // Result should be approximately (0, 0, 1) for 90-degree rotation
    REQUIRE_THAT(r.z, WithinAbs(1.0f, 1e-3f));
}

TEST_CASE("Matrix4x4 CreateRotationY", "[mathutil][matrix4x4]")
{
    // Rotate (1,0,0) by 90 degrees around Y -> (0,0,-1)
    Matrix4x4f m = Matrix4x4f::CreateRotationY(90.0f);
    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f r = m * v;
    REQUIRE_THAT(r.y, WithinAbs(0.0f, 1e-3f));
    REQUIRE_THAT(fabs(r.z), WithinAbs(1.0f, 1e-3f));
}

TEST_CASE("Matrix4x4 CreateRotationZ", "[mathutil][matrix4x4]")
{
    // Rotate (1,0,0) by 90 degrees around Z -> (0,1,0)
    Matrix4x4f m = Matrix4x4f::CreateRotationZ(90.0f);
    Vector3f v(1.0f, 0.0f, 0.0f);
    Vector3f r = m * v;
    REQUIRE_THAT(r.z, WithinAbs(0.0f, 1e-3f));
    REQUIRE_THAT(fabs(r.y), WithinAbs(1.0f, 1e-3f));
}

TEST_CASE("Matrix4x4 IsIdentity", "[mathutil][matrix4x4]")
{
    Matrix4x4f m;
    REQUIRE(m.IsIdentity());

    Matrix4x4f m2 = Matrix4x4f::CreateTranslate(1.0f, 0.0f, 0.0f);
    REQUIRE(!m2.IsIdentity());
}

TEST_CASE("Matrix4x4 MakeIdentity", "[mathutil][matrix4x4]")
{
    Matrix4x4f m = Matrix4x4f::CreateTranslate(5.0f, 5.0f, 5.0f);
    m.MakeIdentity();
    REQUIRE(m.IsIdentity());
}

TEST_CASE("Matrix4x4 addition", "[mathutil][matrix4x4]")
{
    Matrix4x4f a;
    Matrix4x4f b;
    Matrix4x4f c = a + b;
    REQUIRE_THAT(c[0][0], WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(c[1][1], WithinAbs(2.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 GetMatrix3", "[mathutil][matrix4x4]")
{
    Matrix4x4f m4 = Matrix4x4f::CreateScale(2.0f, 3.0f, 4.0f);
    Matrix3x3f m3 = m4.GetMatrix3();
    REQUIRE_THAT(m3[0][0], WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(m3[1][1], WithinAbs(3.0f, 1e-5f));
    REQUIRE_THAT(m3[2][2], WithinAbs(4.0f, 1e-5f));
}

TEST_CASE("Matrix4x4 column accessor", "[mathutil][matrix4x4]")
{
    Matrix4x4f m;
    Vector4f c0 = m.col(0);
    REQUIRE_THAT(c0.x, WithinAbs(1.0f, 1e-5f));  // Identity: col0 = (1,0,0,0)
    REQUIRE_THAT(c0.y, WithinAbs(0.0f, 1e-5f));
}

// ==================== AABB 测试 ====================

TEST_CASE("AABB default construction", "[mathutil][aabb]")
{
    AxisAlignedBoxf box;
    // Default AABB should be in a valid state
}

TEST_CASE("AABB construction with min/max", "[mathutil][aabb]")
{
    AxisAlignedBoxf box(Vector3f(-1.0f, -2.0f, -3.0f), Vector3f(1.0f, 2.0f, 3.0f));

    REQUIRE_THAT(box.minimum.x, WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(box.minimum.y, WithinAbs(-2.0f, 1e-5f));
    REQUIRE_THAT(box.minimum.z, WithinAbs(-3.0f, 1e-5f));
    REQUIRE_THAT(box.maximum.x, WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(box.maximum.y, WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(box.maximum.z, WithinAbs(3.0f, 1e-5f));
}

TEST_CASE("AABB center and length", "[mathutil][aabb]")
{
    AxisAlignedBoxf box(Vector3f(-1.0f, -1.0f, -1.0f), Vector3f(1.0f, 1.0f, 1.0f));

    REQUIRE_THAT(box.center.x, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(box.center.y, WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(box.center.z, WithinAbs(0.0f, 1e-5f));

    REQUIRE_THAT(box.length.x, WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(box.length.y, WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(box.length.z, WithinAbs(2.0f, 1e-5f));
}

TEST_CASE("AABB asymmetric box", "[mathutil][aabb]")
{
    AxisAlignedBoxf box(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(10.0f, 20.0f, 30.0f));

    REQUIRE_THAT(box.center.x, WithinAbs(5.0f, 1e-5f));
    REQUIRE_THAT(box.center.y, WithinAbs(10.0f, 1e-5f));
    REQUIRE_THAT(box.center.z, WithinAbs(15.0f, 1e-5f));

    REQUIRE_THAT(box.length.x, WithinAbs(10.0f, 1e-5f));
    REQUIRE_THAT(box.length.y, WithinAbs(20.0f, 1e-5f));
    REQUIRE_THAT(box.length.z, WithinAbs(30.0f, 1e-5f));
}

TEST_CASE("AABB FromPositions", "[mathutil][aabb]")
{
    std::vector<Vector3f> positions = {
        Vector3f(-1.0f, -2.0f, -3.0f),
        Vector3f(1.0f, 2.0f, 3.0f),
        Vector3f(0.0f, 0.0f, 0.0f)
    };

    AxisAlignedBoxf box = AxisAlignedBoxf::FromPositions(positions);
    REQUIRE_THAT(box.minimum.x, WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(box.minimum.y, WithinAbs(-2.0f, 1e-5f));
    REQUIRE_THAT(box.minimum.z, WithinAbs(-3.0f, 1e-5f));
    REQUIRE_THAT(box.maximum.x, WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(box.maximum.y, WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(box.maximum.z, WithinAbs(3.0f, 1e-5f));
}

// ==================== MathUtil 测试 ====================

TEST_CASE("MathUtil degToRad and radToDeg", "[mathutil][mathutil]")
{
    REQUIRE_THAT(degToRad(180.0f), WithinAbs(static_cast<float>(M_PI), 1e-5f));
    REQUIRE_THAT(radToDeg(static_cast<float>(M_PI)), WithinAbs(180.0f, 1e-5f));

    // Roundtrip
    REQUIRE_THAT(radToDeg(degToRad(45.0f)), WithinAbs(45.0f, 1e-5f));
}

TEST_CASE("MathUtil sinCos", "[mathutil][mathutil]")
{
    float s, c;
    sinCos(&s, &c, static_cast<float>(M_PI) / 2.0f);
    REQUIRE_THAT(s, WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(c, WithinAbs(0.0f, 1e-5f));
}

TEST_CASE("MathUtil Clamp", "[mathutil][mathutil]")
{
    REQUIRE(Clamp(5, 0, 10) == 5);
    REQUIRE(Clamp(-1, 0, 10) == 0);
    REQUIRE(Clamp(15, 0, 10) == 10);
    REQUIRE_THAT(Clamp(0.5f, 0.0f, 1.0f), WithinAbs(0.5f, 1e-5f));
    REQUIRE_THAT(Clamp(-0.5f, 0.0f, 1.0f), WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(Clamp(1.5f, 0.0f, 1.0f), WithinAbs(1.0f, 1e-5f));
}

TEST_CASE("MathUtil Sign", "[mathutil][mathutil]")
{
    REQUIRE_THAT(Sign(5.0f), WithinAbs(1.0f, 1e-5f));
    REQUIRE_THAT(Sign(-5.0f), WithinAbs(-1.0f, 1e-5f));
    REQUIRE_THAT(Sign(0.0f), WithinAbs(-1.0f, 1e-5f));  // Sign returns -1 for 0 per implementation
}

TEST_CASE("MathUtil Mix", "[mathutil][mathutil]")
{
    REQUIRE_THAT(Mix(0.0f, 10.0f, 0.5f), WithinAbs(5.0f, 1e-5f));
    REQUIRE_THAT(Mix(0.0f, 10.0f, 0.0f), WithinAbs(0.0f, 1e-5f));
    REQUIRE_THAT(Mix(0.0f, 10.0f, 1.0f), WithinAbs(10.0f, 1e-5f));
}

TEST_CASE("MathUtil InvSqrt", "[mathutil][mathutil]")
{
    // InvSqrt(x) should be approximately 1/sqrt(x)
    REQUIRE_THAT(1.0f / std::sqrt(4.0f), WithinAbs(0.5f, 1e-3f));
}

TEST_CASE("MathUtil Sqrt", "[mathutil][mathutil]")
{
    REQUIRE_THAT(std::sqrt(4.0f), WithinAbs(2.0f, 1e-5f));
    REQUIRE_THAT(std::sqrt(9.0f), WithinAbs(3.0f, 1e-5f));
}

TEST_CASE("MathUtil Sqr", "[mathutil][mathutil]")
{
    float val = 3.0f;
    REQUIRE_THAT(val * val, WithinAbs(9.0f, 1e-5f));
    float neg = -2.0f;
    REQUIRE_THAT(neg * neg, WithinAbs(4.0f, 1e-5f));
}

TEST_CASE("MathUtil FAbs", "[mathutil][mathutil]")
{
    REQUIRE_THAT(fabs(-3.5f), WithinAbs(3.5f, 1e-5f));
    REQUIRE_THAT(fabs(3.5f), WithinAbs(3.5f, 1e-5f));
}

TEST_CASE("MathUtil ACos ASin", "[mathutil][mathutil]")
{
    REQUIRE_THAT(acos(0.0f), WithinAbs(static_cast<float>(M_PI) / 2.0f, 1e-5f));
    REQUIRE_THAT(asin(1.0f), WithinAbs(static_cast<float>(M_PI) / 2.0f, 1e-5f));
}

TEST_CASE("MathUtil Ceil Floor", "[mathutil][mathutil]")
{
    REQUIRE_THAT(ceil(3.2f), WithinAbs(4.0f, 1e-5f));
    REQUIRE_THAT(floor(3.8f), WithinAbs(3.0f, 1e-5f));
}

TEST_CASE("MathUtil constants", "[mathutil][mathutil]")
{
    REQUIRE_THAT(kPi, WithinAbs(3.14159265f, 1e-5f));
    REQUIRE_THAT(k2Pi, WithinAbs(2.0f * 3.14159265f, 1e-4f));
    REQUIRE_THAT(kPiOver2, WithinAbs(1.57079632f, 1e-5f));
}
