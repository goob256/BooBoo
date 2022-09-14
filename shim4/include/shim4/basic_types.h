#ifndef NOO_TYPES_H
#define NOO_TYPES_H

#include "shim4/main.h"

namespace noo {

namespace util {

template<typename T> class __POINT_BASE__
{
public:
	T x;
	T y;
};

template<typename T> class __SIZE_BASE__
{
public:
	T w;
	T h;
};

template<typename T> class Size : public __SIZE_BASE__<T>
	{
public:
	Size(T w, T h)
	{
		this->w = w;
		this->h = h;
	}

	Size(const Size<int> &s)
	{
		this->w = (T)s.w;
		this->h = (T)s.h;
	}

	Size(const Size<float> &s)
	{
		this->w = (T)s.w;
		this->h = (T)s.h;
	}

	Size() {}

	float length()
	{
		float dx = (float)this->w;
		float dy = (float)this->h;
		return sqrtf(dx*dx + dy*dy);
	}

	float angle()
	{
		float dx = (float)this->w;
		float dy = (float)this->h;
		return atan2f(dy, dx);
	}

	T area()
	{
		return this->w * this->h;
	}

	bool is_in_bounds(const __POINT_BASE__<T> &p)
	{
		return p.x >= 0 && p.y >= 0 && p.x < this->w && p.y < this->h;
	}

	void operator=(const Size<T> &from)
	{
		this->w = from.w;
		this->h = from.h;
	}

	inline bool operator==(const Size<T> &rhs)
	{
		return this->w == rhs.w && this->h == rhs.h;
	}

	inline bool operator!=(const Size<T> &rhs)
	{
		return this->w != rhs.w || this->h != rhs.h;
	}

	inline Size<T> operator+(const T &rhs)
	{
		Size<T> s;
		s.w = this->w + rhs;
		s.h = this->h + rhs;
		return s;
	}

	inline Size<T> operator-(const T &rhs)
	{
		Size<T> s;
		s.w = this->w - rhs;
		s.h = this->h - rhs;
		return s;
	}

	inline Size<T> operator*(const T &rhs)
	{
		Size<T> s;
		s.w = this->w * rhs;
		s.h = this->h * rhs;
		return s;
	}

	inline Size<T> operator/(const T &rhs)
	{
		Size<T> s;
		s.w = this->w / rhs;
		s.h = this->h / rhs;
		return s;
	}

	inline Size<T> &operator+=(const T &rhs)
	{
		this->w += rhs;
		this->h += rhs;
		return *this;
	}

	inline Size<T> &operator-=(const T &rhs)
	{
		this->w -= rhs;
		this->h -= rhs;
		return *this;
	}

	inline Size<T> &operator*=(const T &rhs)
	{
		this->w *= rhs;
		this->h *= rhs;
		return *this;
	}

	inline Size<T> &operator/=(const T &rhs)
	{
		this->w /= rhs;
		this->h /= rhs;
		return *this;
	}

	inline Size<T> &operator-=(const Size<T> &rhs)
	{
		this->w -= rhs.w;
		this->h -= rhs.h;
		return *this;
	}

	inline Size<T> &operator+=(const Size<T> &rhs)
	{
		this->w += rhs.w;
		this->h += rhs.h;
		return *this;
	}

	inline Size<T> &operator/=(const Size<T> &rhs)
	{
		this->w /= rhs.w;
		this->h /= rhs.h;
		return *this;
	}

	inline Size<T> &operator*=(const Size<T> &rhs)
	{
		this->w *= rhs.w;
		this->h *= rhs.h;
		return *this;
	}

	inline Size<T> operator-(const Size<T> &rhs)
	{
		Size<T> s(this->w - rhs.w, this->h - rhs.h);
		return s;
	}

	inline Size<T> operator+(const Size<T> &rhs)
	{
		Size<T> s(this->w + rhs.w, this->h + rhs.h);
		return s;
	}

	inline Size<T> operator/(const Size<T> &rhs)
	{
		Size<T> s(this->w / rhs.w, this->h / rhs.h);
		return s;
	}

	inline Size<T> operator*(const Size<T> &rhs)
	{
		Size<T> s(this->w * rhs.w, this->h * rhs.h);
		return s;
	}

	inline Size<T> operator-()
	{
		Size<T> s;
		s.w = -this->w;
		s.h = -this->h;
		return s;
	}
};

template<typename T> class Point : public __POINT_BASE__<T>
{
public:
	Point()
	{
		this->x = this->y = 0;
	}

	Point(T x, T y)
	{
		this->x = x;
		this->y = y;
	}

	Point(const Point<int> &p)
	{
		this->x = (T)p.x;
		this->y = (T)p.y;
	}

	Point(const Point<float> &p)
	{
		this->x = (T)p.x;
		this->y = (T)p.y;
	}

	float length()
	{
		float dx = (float)this->x;
		float dy = (float)this->y;
		return sqrtf(dx*dx + dy*dy);
	}

	float angle()
	{
		float dx = (float)this->x;
		float dy = (float)this->y;
		return atan2f(dy, dx);
	}

	inline void operator=(const Point<T> &from)
	{
		this->x = from.x;
		this->y = from.y;
	}

	inline bool operator==(const Point<T> &rhs)
	{
		return this->x == rhs.x && this->y == rhs.y;
	}

	inline bool operator!=(const Point<T> &rhs)
	{
		return this->x != rhs.x || this->y != rhs.y;
	}

	inline Point<T> operator+(const T &rhs)
	{
		Point<T> p;
		p.x = this->x + rhs;
		p.y = this->y + rhs;
		return p;
	}

	inline Point<T> operator-(const T &rhs)
	{
		Point<T> p;
		p.x = this->x - rhs;
		p.y = this->y - rhs;
		return p;
	}

	inline Point<T> operator*(const T &rhs)
	{
		Point<T> p;
		p.x = this->x * rhs;
		p.y = this->y * rhs;
		return p;
	}

	inline Point<T> operator/(const T &rhs)
	{
		Point<T> p;
		p.x = this->x / rhs;
		p.y = this->y / rhs;
		return p;
	}

	inline Point<T> &operator+=(const T &rhs)
	{
		this->x += rhs;
		this->y += rhs;
		return *this;
	}

	inline Point<T> &operator-=(const T &rhs)
	{
		this->x -= rhs;
		this->y -= rhs;
		return *this;
	}

	inline Point<T> &operator*=(const T &rhs)
	{
		this->x *= rhs;
		this->y *= rhs;
		return *this;
	}

	inline Point<T> &operator/=(const T &rhs)
	{
		this->x /= rhs;
		this->y /= rhs;
		return *this;
	}

	inline Point<T> operator+(const Point<T> &rhs)
	{
		Point<T> p;
		p.x = this->x + rhs.x;
		p.y = this->y + rhs.y;
		return p;
	}

	inline Point<T> operator-(const Point<T> &rhs)
	{
		Point<T> p;
		p.x = this->x - rhs.x;
		p.y = this->y - rhs.y;
		return p;
	}

	inline Point<T> operator*(const Point<T> &rhs)
	{
		Point<T> p;
		p.x = this->x * rhs.x;
		p.y = this->y * rhs.y;
		return p;
	}

	inline Point<T> operator/(const Point<T> &rhs)
	{
		Point<T> p;
		p.x = this->x / rhs.x;
		p.y = this->y / rhs.y;
		return p;
	}

	inline Point<T> &operator+=(const Point<T> &rhs)
	{
		this->x += rhs.x;
		this->y += rhs.y;
		return *this;
	}

	inline Point<T> &operator-=(const Point<T> &rhs)
	{
		this->x -= rhs.x;
		this->y -= rhs.y;
		return *this;
	}

	inline Point<T> &operator*=(const Point<T> &rhs)
	{
		this->x *= rhs.x;
		this->y *= rhs.y;
		return *this;
	}

	inline Point<T> &operator/=(const Point<T> &rhs)
	{
		this->x /= rhs.x;
		this->y /= rhs.y;
		return *this;
	}

	inline Point<T> operator+(const Size<T> &rhs)
	{
		Point<T> p;
		p.x = this->x + rhs.w;
		p.y = this->y + rhs.h;
		return p;
	}

	inline Point<T> operator-(const Size<T> &rhs)
	{
		Point<T> p;
		p.x = this->x - rhs.w;
		p.y = this->y - rhs.h;
		return p;
	}

	inline Point<T> operator*(const Size<T> &rhs)
	{
		Point<T> p;
		p.x = this->x * rhs.w;
		p.y = this->y * rhs.h;
		return p;
	}

	inline Point<T> operator/(const Size<T> &rhs)
	{
		Point<T> p;
		p.x = this->x / rhs.w;
		p.y = this->y / rhs.h;
		return p;
	}

	inline Point<T> &operator+=(const Size<T> &rhs)
	{
		this->x += rhs.w;
		this->y += rhs.h;
		return *this;
	}

	inline Point<T> &operator-=(const Size<T> &rhs)
	{
		this->x -= rhs.w;
		this->y -= rhs.h;
		return *this;
	}

	inline Point<T> &operator*=(const Size<T> &rhs)
	{
		this->x *= rhs.w;
		this->y *= rhs.h;
		return *this;
	}

	inline Point<T> &operator/=(const Size<T> &rhs)
	{
		this->x /= rhs.w;
		this->y /= rhs.h;
		return *this;
	}

	inline Point<T> operator-()
	{
		Point<T> p;
		p.x = -this->x;
		p.y = -this->y;
		return p;
	}
};

template<typename T> class Vec3D
	{
public:
	T x;
	T y;
	T z;

	Vec3D(T x, T y, T z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	Vec3D(const Vec3D<int> &s)
	{
		this->x = (T)s.x;
		this->y = (T)s.y;
		this->z = (T)s.z;
	}

	Vec3D(const Vec3D<float> &s)
	{
		this->x = (T)s.x;
		this->y = (T)s.y;
		this->z = (T)s.z;
	}

	Vec3D()
	{
		x = y = z = 0;
	}

	void operator=(const Vec3D<T> &from)
	{
		x = from.x;
		y = from.y;
		z = from.z;
	}

	inline Vec3D<T> operator+(const T &rhs)
	{
		Vec3D<T> s;
		s.x = this->x + rhs;
		s.y = this->y + rhs;
		s.z = this->z + rhs;
		return s;
	}

	inline Vec3D<T> operator-(const T &rhs)
	{
		Vec3D<T> s;
		s.x = this->x - rhs;
		s.y = this->y - rhs;
		s.z = this->z - rhs;
		return s;
	}

	inline Vec3D<T> operator*(const T &rhs)
	{
		Vec3D<T> s;
		s.x = this->x * rhs;
		s.y = this->y * rhs;
		s.z = this->z * rhs;
		return s;
	}

	inline Vec3D<T> operator/(const T &rhs)
	{
		Vec3D<T> s;
		s.x = this->x / rhs;
		s.y = this->y / rhs;
		s.z = this->z / rhs;
		return s;
	}

	inline Vec3D<T> &operator+=(const T &rhs)
	{
		this->x += rhs;
		this->y += rhs;
		this->z += rhs;
		return *this;
	}

	inline Vec3D<T> &operator-=(const T &rhs)
	{
		this->x -= rhs;
		this->y -= rhs;
		this->z -= rhs;
		return *this;
	}

	inline Vec3D<T> &operator*=(const T &rhs)
	{
		this->x *= rhs;
		this->y *= rhs;
		this->z *= rhs;
		return *this;
	}

	inline Vec3D<T> &operator/=(const T &rhs)
	{
		this->x /= rhs;
		this->y /= rhs;
		this->z /= rhs;
		return *this;
	}

	inline Vec3D<T> &operator-=(const Vec3D<T> &rhs)
	{
		this->x -= rhs.x;
		this->y -= rhs.y;
		this->z -= rhs.z;
		return *this;
	}

	inline Vec3D<T> &operator+=(const Vec3D<T> &rhs)
	{
		this->x += rhs.x;
		this->y += rhs.y;
		this->z += rhs.z;
		return *this;
	}

	inline Vec3D<T> &operator/=(const Vec3D<T> &rhs)
	{
		this->x /= rhs.x;
		this->y /= rhs.y;
		this->z /= rhs.z;
		return *this;
	}

	inline Vec3D<T> &operator*=(const Vec3D<T> &rhs)
	{
		this->x *= rhs.x;
		this->y *= rhs.y;
		this->z *= rhs.z;
		return *this;
	}

	inline Vec3D<T> operator-(const Vec3D<T> &rhs)
	{
		Vec3D<T> s(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z);
		return s;
	}

	inline Vec3D<T> operator+(const Vec3D<T> &rhs)
	{
		Vec3D<T> s(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z);
		return s;
	}

	inline Vec3D<T> operator/(const Vec3D<T> &rhs)
	{
		Vec3D<T> s(this->x / rhs.x, this->y / rhs.y, this->z / rhs.z);
		return s;
	}

	inline Vec3D<T> operator*(const Vec3D<T> &rhs)
	{
		Vec3D<T> s(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z);
		return s;
	}

	inline Vec3D<T> operator-()
	{
		Vec3D<T> s;
		s.x = -this->x;
		s.y = -this->y;
		s.z = -this->z;
		return s;
	}

	float dot(const Vec3D<T> &v)
	{
		return x*v.x + y*v.y + z*v.z;
	}

	float length()
	{
		return sqrtf(dot(*this));
	}

	Vec3D<T> normalize()
	{
		return *this / length();
	}

	float distance(const Vec3D<T> &v)
	{
		return (*this - v).length();
	}

	float angle(Vec3D<T> &v)
	{
		return acosf(dot(v) / (*this * v.length()).length());
	}

	float angle()
	{
		return angle(Vec3D<T>(1, 0, 0));
	}

	Vec3D<T> project(Vec3D<T> &v)
	{
		return v * (dot(v)/v.dot(v));
	}

	Vec3D<T> cross(Vec3D<T> &v)
	{
		return Vec3D<T>(x*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
	}
};

template<typename T> class Rectangle
{
public:
	Rectangle()
	{
	}

	Rectangle(Point<T> pos, Size<T> size) :
		pos(pos),
		size(size)
	{
	}

	bool contains(Point<T> p)
	{
		return (p.x >= pos.x && p.y >= pos.y && p.x < pos.x+size.w && p.y < pos.y+size.h);
	}

	bool contains(Point<T> p, Size<T> s)
	{
		return (p.x >= pos.x && p.y >= pos.y && p.x+s.x <= pos.x+size.w && p.y+s.y <= pos.y+size.h);
	}

	Point<T> pos;
	Size<T> size;	
};

} // End namespace util

} // End namespace noo

#endif // NOO_TYPES_H
