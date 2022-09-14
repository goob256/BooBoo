#include "shim4/interp.h"

using namespace noo;

namespace noo {

namespace math {

Interpolator::~Interpolator()
{
}

float Interpolator::get_value()
{
	return _value;
}

I_Hermite::~I_Hermite()
{
}

bool I_Hermite::start(float prev, float from, float to, float next, int ticks)
{
	_value = from;
	_ticks = ticks;
	_curr_tick = 0;
	_from = from;
	_to = to;
	_prev = prev;
	_next = next;

	return true;
}

bool I_Hermite::interpolate(int t)
{
	_curr_tick += (t-1);
	float p = _curr_tick / (float)_ticks; // FIXME: ticks-1?
	// From Game Programming Gems Volume 1 pg. 266 for Catmull-Rom Splines
	_value = _prev * (-0.5f*p*p*p + p*p - 0.5f*p) +
		_from * (1.5f*p*p*p + -2.5f*p*p + 1.0f) +
		_to * (-1.5f*p*p*p + 2.0f*p*p + 0.5f*p) +
		_next * (0.5f*p*p*p - 0.5f*p*p);
	_curr_tick++;
	return _curr_tick < _ticks;
}

I_Basic::~I_Basic()
{
}

bool I_Basic::start(float prev, float from, float to, float next, int ticks)
{
	_value = from;
	_ticks = ticks;
	_curr_tick = 0;
	_from = from;
	_to = to;
	return true;
}

I_Linear::~I_Linear()
{
}

bool I_Linear::interpolate(int t)
{
	_curr_tick += (t-1);
	_value = _curr_tick / (float)_ticks;
	_value = _value * (_to-_from) + _from;
	_curr_tick++;
	return _curr_tick < _ticks;
}

I_Slow::~I_Slow()
{
}

bool I_Slow::interpolate(int t)
{
	_curr_tick += (t-1);
	_value = _curr_tick / (float)_ticks;
	_value = _value * _value;
	_value = _value * (_to-_from) + _from;
	_curr_tick++;
	return _curr_tick < _ticks;
}

I_Sin::~I_Sin()
{
}

bool I_Sin::interpolate(int t)
{
	_curr_tick += (t-1);
	_value = _curr_tick / (float)_ticks;
	if (_from <= _to) {
		_value = sinf(_value * M_PI / 2.0f);
	}
	else {
		_value = (1.0f - sinf(_value * M_PI / 2.0f + M_PI / 2.0f));
	}
	_value = _value * (_to-_from) + _from;
	_curr_tick++;
	return _curr_tick < _ticks;
}

I_One::~I_One()
{
}

bool I_One::start(float prev, float from, float to, float next, int ticks)
{
	bool ret = I_Basic::start(prev, from, to, next, ticks);
	_value = to;
	return ret;
}

bool I_One::interpolate(int t)
{
	// _value is always 1.0f, set in start
	_curr_tick += t;
	return _curr_tick < _ticks;
}

I_Zero::~I_Zero()
{
}

bool I_Zero::start(float prev, float from, float to, float next, int ticks)
{
	bool ret = I_Basic::start(prev, from, to, next, ticks);
	_value = from;
	return ret;
}

bool I_Zero::interpolate(int t)
{
	// _value always start at zero... see I_Basic::start
	_curr_tick += t;
	return _curr_tick < _ticks;
}

I_Pulse::~I_Pulse()
{
}

bool I_Pulse::start(float prev, float from, float to, float next, int ticks)
{
	bool ret = I_Basic::start(prev, from, to, next, ticks);
	_value = from;
	return ret;
}

bool I_Pulse::interpolate(int t)
{
	// _value always start at zero... see I_Basic::start
	_curr_tick += t;
	if (_curr_tick < _ticks/2) {
		_value = _from;
	}
	else {
		_value = _to;
	}
	return _curr_tick < _ticks;
}

} // End namespace math

} // End namespace noo
