#pragma once

#ifndef COLOR_H
#define COLOR_H

#include <DirectXMath.h>

using namespace DirectX;

class Color {
 public:
  Color( ) : value_(g_XMOne) {}
  Color( FXMVECTOR vec );
  Color( const XMVECTORF32& vec );
  Color( float r, float g, float b, float a = 1.0f );
  Color( uint16_t r, uint16_t g, uint16_t b, uint16_t a = 255, uint16_t bitDepth = 8 );
  explicit Color( uint32_t rgbaLittleEndian );

  float R() const { return XMVectorGetX(value_); }
  float G() const { return XMVectorGetY(value_); }
  float B() const { return XMVectorGetZ(value_); }
  float A() const { return XMVectorGetW(value_); }

  bool operator==( const Color& rhs ) const { return XMVector4Equal(value_, rhs.value_); }
  bool operator!=( const Color& rhs ) const { return !XMVector4Equal(value_, rhs.value_); }

  void SetR( float r ) { value_.f[0] = r; }
  void SetG( float g ) { value_.f[1] = g; }
  void SetB( float b ) { value_.f[2] = b; }
  void SetA( float a ) { value_.f[3] = a; }

  float* Ptr( void ) { return reinterpret_cast<float*>(this); }
  float& operator[]( int idx ) { return Ptr()[idx]; }

 private:
  XMVECTORF32 value_;
};

inline Color::Color( FXMVECTOR vec )
{
  value_.v = vec;
}

inline Color::Color( const XMVECTORF32& vec )
{
  value_ = vec;
}

inline Color::Color( float r, float g, float b, float a )
{
  value_.v = XMVectorSet(r, g, b, a);
}

inline Color::Color( uint16_t r, uint16_t g, uint16_t b, uint16_t a, uint16_t bitDepth )
{
  value_.v = XMVectorScale(XMVectorSet(r, g, b, a), 1.0f / ((1 << bitDepth) - 1));
}

inline Color::Color( uint32_t u32 )
{
  float r = (float)((u32 >>  0) & 0xFF);
  float g = (float)((u32 >>  8) & 0xFF);
  float b = (float)((u32 >> 16) & 0xFF);
  float a = (float)((u32 >> 24) & 0xFF);
  value_.v = XMVectorScale( XMVectorSet(r, g, b, a), 1.0f / 255.0f );
}

#endif // !COLOR_H

