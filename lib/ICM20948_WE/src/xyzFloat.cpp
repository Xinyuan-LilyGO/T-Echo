/* defines the structure xyzFloat which is used for gyroscopes, accelerometers and 
   magnetometers such as ICM20948, MPU9250, ADXL345, etc. */

#include "xyzFloat.h"

xyzFloat::xyzFloat()
    : xyzFloat(0.f, 0.f, 0.f)
{
    // intentionally empty
}

xyzFloat::xyzFloat(float const x, float const y, float const z)
    : x(x)
    , y(y)
    , z(z)
{
    // intentionally empty
}

xyzFloat xyzFloat::operator+() const
{
    return *this;
}

xyzFloat xyzFloat::operator-() const
{
    return xyzFloat{-x,
                    -y,
                    -z};
}

xyzFloat xyzFloat::operator+(xyzFloat const & summand) const
{
    return xyzFloat{x + summand.x,
                    y + summand.y,
                    z + summand.z};
}

xyzFloat xyzFloat::operator-(xyzFloat const & subtrahend) const
{
    return xyzFloat{x - subtrahend.x,
                    y - subtrahend.y,
                    z - subtrahend.z};
}

xyzFloat xyzFloat::operator*(float const operand) const
{
    return xyzFloat{x * operand,
                    y * operand,
                    z * operand};
}

xyzFloat xyzFloat::operator/(float const divisor) const
{
    return xyzFloat{x / divisor,
                    y / divisor,
                    z / divisor};
}

xyzFloat & xyzFloat::operator+=(xyzFloat const & summand)
{
    x += summand.x;
    y += summand.y;
    z += summand.z;
    return *this;
}

xyzFloat & xyzFloat::operator-=(xyzFloat const & subtrahend)
{
    x -= subtrahend.x;
    y -= subtrahend.y;
    z -= subtrahend.z;
    return *this;
}

xyzFloat & xyzFloat::operator*=(float const operand)
{
    x *= operand;
    y *= operand;
    z *= operand;
    return *this;
}

xyzFloat & xyzFloat::operator/=(float const divisor)
{
    x /= divisor;
    y /= divisor;
    z /= divisor;
    return *this;
}
