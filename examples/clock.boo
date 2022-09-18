var string reset_game_name
= reset_game_name "blip.boo"
include "slideshow_start.inc"

var number ticks
= ticks 0

var number success
cfg_load success "com.b1stable.clock"
? success 0
je create
cfg_get_number ticks "ticks"
goto done_cfg
:create
cfg_set_number "ticks" ticks
:done_cfg

function logic
start
	+ ticks 1
	include "slideshow_logic.inc"
end

function draw
start
	clear 255 255 255

	var number ninety
	= ninety 3.14159
	/ ninety 2

	var number angle
	var number angle2
	var number angle3

	= angle ticks
	/ angle 60
	/ angle 60
	= angle2 angle
	* angle 3.14159
	* angle 2
	- angle ninety

	/ angle2 60
	= angle3 angle2
	* angle2 3.14159
	* angle2 2
	- angle2 ninety

	/ angle3 12
	* angle3 2
	* angle3 3.14159
	- angle3 ninety

	var number x
	var number y

	cos x angle2
	* x 85
	+ x 320
	sin y angle2
	* y 85
	+ y 180

	line 0 0 0 255 x y 320 180 5 ; minute hand
	
	var number a1
	var number a2
	var number x1
	var number y1
	var number x2
	var number y2
	var number x3
	var number y3

	cos x1 angle2
	* x1 5
	+ x1 x
	sin y1 angle2
	* y1 5
	+ y1 y
	= a1 angle2
	+ a1 ninety
	= a2 angle2
	- a2 ninety
	cos x2 a1
	sin y2 a1
	* x2 2.5
	* y2 2.5
	+ x2 x
	+ y2 y
	cos x3 a2
	sin y3 a2
	* x3 2.5
	* y3 2.5
	+ x3 x
	+ y3 y

	filled_triangle 0 0 0 255 0 0 0 255 0 0 0 255 x1 y1 x2 y2 x3 y3 ; minute hand point

	var number a2
	= a2 angle2
	+ a2 3.14159
	cos x a2
	* x 10
	+ x 320
	sin y a2
	* y 10
	+ y 180

	line 0 0 0 255 x y 320 180 5 ; extend minute hand back

	cos x angle3
	* x 60
	+ x 320
	sin y angle3
	* y 60
	+ y 180

	line 0 0 0 255 x y 320 180 10 ; hour hand
	
	cos x1 angle3
	* x1 10
	+ x1 x
	sin y1 angle3
	* y1 10
	+ y1 y
	= a1 angle3
	+ a1 ninety
	= a2 angle3
	- a2 ninety
	cos x2 a1
	sin y2 a1
	* x2 5
	* y2 5
	+ x2 x
	+ y2 y
	cos x3 a2
	sin y3 a2
	* x3 5
	* y3 5
	+ x3 x
	+ y3 y

	filled_triangle 0 0 0 255 0 0 0 255 0 0 0 255 x1 y1 x2 y2 x3 y3 ; hour hand point
	
	= a2 angle3
	+ a2 3.14159
	cos x a2
	* x 10
	+ x 320
	sin y a2
	* y 10
	+ y 180

	line 0 0 0 255 x y 320 180 10 ; extend hour hand back

	cos x angle
	* x 100
	+ x 320
	sin y angle
	* y 100
	+ y 180

	line 255 0 0 255 x y 320 180 2 ; second hand

	var number a2
	= a2 angle
	+ a2 3.14159
	cos x a2
	* x 15
	+ x 320
	sin y a2
	* y 15
	+ y 180

	line 255 0 0 255 x y 320 180 2 ; extend second hand back
end

function shutdown
start
	cfg_set_number "ticks" ticks
	cfg_save "com.b1stable.clock"
end
