#ifndef NOO_INTERP_H
#define NOO_INTERP_H

#include "shim4/main.h"

namespace noo {

namespace math {

class Interpolator
{
public:
	virtual ~Interpolator();

	virtual bool start(float prev, float from, float to, float next, int ticks) = 0;
	virtual bool interpolate(int t) = 0;
	float get_value();

protected:
	float _value; // 0-1
	int _ticks;
	int _curr_tick;
	float _from;
	float _to;
};

// 1 in MML
class I_Hermite : public Interpolator
{
public:
	virtual ~I_Hermite();

	bool start(float prev, float from, float to, float next, int ticks);
	bool interpolate(int t);

private:
	float _prev;
	float _next;
};

class I_Basic : public Interpolator
{
public:
	virtual ~I_Basic();

	bool start(float prev, float from, float to, float next, int ticks);

protected:
};

// 0 in MML
class I_Linear : public I_Basic
{
public:
	virtual ~I_Linear();

	bool interpolate(int t);
};

// 2 in MML
class I_Slow : public I_Basic
{
public:
	virtual ~I_Slow();

	bool interpolate(int t);
};

// 3 in MML
class I_Sin : public I_Basic
{
public:
	virtual ~I_Sin();

	bool interpolate(int t);
};

class I_One : public I_Basic
{
public:
	virtual ~I_One();

	bool start(float prev, float from, float to, float next, int ticks);
	bool interpolate(int t);
};

class I_Zero : public I_Basic
{
public:
	virtual ~I_Zero();

	bool start(float prev, float from, float to, float next, int ticks);
	bool interpolate(int t);
};

class I_Pulse : public I_Basic
{
public:
	virtual ~I_Pulse();

	bool start(float prev, float from, float to, float next, int ticks);
	bool interpolate(int t);
};

} // End namespace math

} // End namespace noo

#endif // NOO_INTERP_H
