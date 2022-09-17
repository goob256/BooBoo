var string reset_game_name
= reset_game_name "pong.boo"
include "slideshow_start.inc"

var number x
var number y

var string cfg_name
= cfg_name "com.b1stable.memory"

var number found
cfg_load found cfg_name
? found 0
jne cfg_found
= x 320
= y 180
goto done_cfg
:cfg_found
cfg_get_number x "x"
cfg_get_number y "y"
:done_cfg

var number radius
= radius 64

function draw
start
	clear 255 0 255

	filled_circle 255 255 0 255 x y radius
end

function logic
start
	include "poll_joystick.inc"

	? joy_x1 0.1
	jg x_ok
	? joy_x1 -0.1
	jl x_ok
	= joy_x1 0
:x_ok

	? joy_y1 0.1
	jg y_ok
	? joy_y1 -0.1
	jl y_ok
	= joy_y1 0
:y_ok

	* joy_x1 5
	* joy_y1 5
	+ x joy_x1
	+ y joy_y1

	? x radius
	jge check_right
	= x radius

:check_right
	var number right
	= right 640
	- right radius
	? x right
	jle check_top
	= x right

:check_top
	? y radius
	jge check_bottom
	= y radius

:check_bottom
	var number bottom
	= bottom 360
	- bottom radius
	? y bottom
	jle finish
	= y bottom

:finish

	cfg_set_number "x" x
	cfg_set_number "y" y

	include "slideshow_logic.inc"
end

function shutdown
start
	cfg_save cfg_name
end
