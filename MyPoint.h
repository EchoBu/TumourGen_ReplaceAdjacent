#pragma once

class MyPoint
{
public:
	union {
		struct { /*数组索引*/double p[3]; };
		struct { /*变量索引*/double x, y, z; };
	};

	MyPoint() {}
	MyPoint(double _x, double _y, double _z) :
		x(_x), y(_y), z(_z)
	{}

	MyPoint operator+(const MyPoint &b) const
	{
		return MyPoint{ x + b.x, y + b.y, z + b.z };
	}
	MyPoint& operator+=(const MyPoint &b)
	{
		x += b.x, y += b.y, z += b.z;
		return *this;
	}
	MyPoint operator-(const MyPoint &b) const
	{
		return MyPoint{ x - b.x, y - b.y, z - b.z };
	}
	MyPoint operator-() const
	{
		return MyPoint(-x, -y, -z);
	}
	MyPoint& operator-=(const MyPoint &b)
	{
		x -= b.x, y -= b.y, z -= b.z;
		return *this;
	}
	MyPoint operator*(double p) const
	{
		return MyPoint{ p*x, p*y, p*z };
	}
	MyPoint& operator*=(const MyPoint &p)
	{
		x *= p.x, y *= p.y, z *= p.z;
		return *this;
	}
	MyPoint& operator*=(double p)
	{
		x *= p, y *= p, z *= p;
		return *this;
	}
	MyPoint& operator/=(double p)
	{
		x /= p, y /= p, z /= p;
		return *this;
	}
	friend MyPoint operator*(double p, const MyPoint &x)
	{
		return MyPoint{ p*x.x, p*x.y, p*x.z };
	}
	MyPoint operator/(double p) const
	{
		return MyPoint{ x / p,  y / p,  z / p };
	}
	friend double dot(const MyPoint &a, const MyPoint &b)
	{
		return a.x * b.x + a.y*b.y + a.z*b.z;
	}
	friend MyPoint cross(const MyPoint &a, const MyPoint &b)
	{
		return MyPoint{
			a.y*b.z - a.z*b.y,
			a.z*b.x - a.x*b.z,
			a.x*b.y - a.y*b.x
		};
	}
	friend MyPoint cross(const MyPoint &a, const MyPoint &o, const MyPoint &b)
	{
		return cross(a - o, b - o);
	}
	friend double norm(const MyPoint &a)
	{
		return a.x*a.x + a.y*a.y + a.z*a.z;
	}
	friend double abs(const MyPoint &a)
	{
		return sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
	}
};