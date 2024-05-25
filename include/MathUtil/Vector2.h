#ifndef MATH3D_VECTOR2_INCLUDE_H_56FG4HJ
#define MATH3D_VECTOR2_INCLUDE_H_56FG4HJ

#include "Math3DCommon.h"

NS_MATHUTIL_BEGIN

template <typename T>
class MATH3D_API Vector2
{
public:
	Vector2();

    Vector2(const T fX, const T fY );

    inline explicit Vector2( const T scaler );

    inline explicit Vector2( const T afCoordinate[2] );

    explicit Vector2( const int afCoordinate[2] );

    inline explicit Vector2( T* const r );

	~Vector2(void);

	/** Exchange the contents of this vector with another. 
	*/
	void Swap(Vector2& other);

	inline T operator [] ( const size_t i ) const
    {
        assert( i < 2 );

        return *(&x+i);
    }

	inline T& operator [] ( const size_t i )
    {
        assert( i < 2 );

        return *(&x+i);
    }

	/// Pointer accessor for direct copying
	inline T* ptr()
	{
		return &x;
	}
	/// Pointer accessor for direct copying
	inline const T* ptr() const
	{
		return &x;
	}

    inline Vector2& operator = ( const Vector2& rkVector )
    {
        x = rkVector.x;
        y = rkVector.y;

        return *this;
    }

	inline Vector2& operator = ( const Real fScalar)
	{
		x = fScalar;
		y = fScalar;

		return *this;
	}

    inline bool operator == ( const Vector2& rkVector ) const
    {
        return ( x == rkVector.x && y == rkVector.y );
    }

    inline bool operator != ( const Vector2& rkVector ) const
    {
        return ( x != rkVector.x || y != rkVector.y  );
    }

    inline Vector2 operator + ( const Vector2& rkVector ) const
    {
        return Vector2(
            x + rkVector.x,
            y + rkVector.y);
    }

    inline Vector2 operator - ( const Vector2& rkVector ) const
    {
        return Vector2(
            x - rkVector.x,
            y - rkVector.y);
    }

    inline Vector2 operator * ( const Real fScalar ) const
    {
        return Vector2(
            x * fScalar,
            y * fScalar);
    }

    inline Vector2 operator * ( const Vector2& rhs) const
    {
        return Vector2(
            x * rhs.x,
            y * rhs.y);
    }

    inline Vector2 operator / ( const Real fScalar ) const
    {
        assert( fScalar != 0.0 );

        Real fInv = 1.0f / fScalar;

        return Vector2(
            x * fInv,
            y * fInv);
    }

    inline Vector2 operator / ( const Vector2& rhs) const
    {
        return Vector2(
            x / rhs.x,
            y / rhs.y);
    }

    inline const Vector2& operator + () const
    {
        return *this;
    }

    inline Vector2 operator - () const
    {
        return Vector2(-x, -y);
    }

    inline friend Vector2 operator * ( const Real fScalar, const Vector2& rkVector )
    {
        return Vector2(
            fScalar * rkVector.x,
            fScalar * rkVector.y);
    }

    inline friend Vector2 operator / ( const Real fScalar, const Vector2& rkVector )
    {
        return Vector2(
            fScalar / rkVector.x,
            fScalar / rkVector.y);
    }

    inline friend Vector2 operator + (const Vector2& lhs, const Real rhs)
    {
        return Vector2(
            lhs.x + rhs,
            lhs.y + rhs);
    }

    inline friend Vector2 operator + (const Real lhs, const Vector2& rhs)
    {
        return Vector2(
            lhs + rhs.x,
            lhs + rhs.y);
    }

    inline friend Vector2 operator - (const Vector2& lhs, const Real rhs)
    {
        return Vector2(
            lhs.x - rhs,
            lhs.y - rhs);
    }

    inline friend Vector2 operator - (const Real lhs, const Vector2& rhs)
    {
        return Vector2(
            lhs - rhs.x,
            lhs - rhs.y);
    }

    // arithmetic updates
    inline Vector2& operator += ( const Vector2& rkVector )
    {
        x += rkVector.x;
        y += rkVector.y;

        return *this;
    }

    inline Vector2& operator += ( const Real fScaler )
    {
        x += fScaler;
        y += fScaler;

        return *this;
    }

    inline Vector2& operator -= ( const Vector2& rkVector )
    {
        x -= rkVector.x;
        y -= rkVector.y;

        return *this;
    }

    inline Vector2& operator -= ( const Real fScaler )
    {
        x -= fScaler;
        y -= fScaler;

        return *this;
    }

    inline Vector2& operator *= ( const Real fScalar )
    {
        x *= fScalar;
        y *= fScalar;

        return *this;
    }

    inline Vector2& operator *= ( const Vector2& rkVector )
    {
        x *= rkVector.x;
        y *= rkVector.y;

        return *this;
    }

    inline Vector2& operator /= ( const Real fScalar )
    {
        assert( fScalar != 0.0 );

        Real fInv = 1.0f / fScalar;

        x *= fInv;
        y *= fInv;

        return *this;
    }

    inline Vector2& operator /= ( const Vector2& rkVector )
    {
        x /= rkVector.x;
        y /= rkVector.y;

        return *this;
    }

    T Length() const;

    T SquaredLength() const;

    T Distance(const Vector2& rhs) const;

    T SquaredDistance(const Vector2& rhs) const;

    T DotProduct(const Vector2& vec) const;

    T Normalise();

    T CrossProduct( const Vector2& rkVector ) const;

    /** Calculates a reflection vector to the plane with the given normal .
    @remarks NB assumes 'this' is pointing AWAY FROM the plane, invert if it is not.
    */
    Vector2<T> Reflect(const Vector2& normal) const;

    T x;
    T y;
};

typedef Vector2<float> Vector2f;
typedef Vector2<float> Vector2d;

NS_MATHUTIL_END

#endif
