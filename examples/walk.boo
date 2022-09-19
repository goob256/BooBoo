var string reset_game_name
= reset_game_name "sneaky.boo"
include "slideshow_start.inc"

var number x
= x 320
var number y
= y 180
var number angle
= angle 0
var number scale
= scale 1

var string cfg_name
= cfg_name "com.b1stable.walk"

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
cfg_get_number angle "angle"
cfg_get_number scale "scale"
:done_cfg

var vector anim
var number img
image_load img "misc/walk1.tga"
vector_add anim img
image_load img "misc/walk2.tga"
vector_add anim img
image_load img "misc/walk3.tga"
vector_add anim img
var number frame
= frame 0
var number frame_timer
= frame_timer 0
var number scale_inc
= scale_inc 1
var number scale_dest
= scale_dest 15
var number moving
= moving 0

function draw
start
	clear 50 60 57

	var number img

	? moving 0
	je static
	vector_get anim img frame
	goto do_draw

:static
	vector_get anim img 1

:do_draw

	; FIXME
	var number sx
	= sx 2.5
	var number sy
	= sy 2.5

	var number s
	= s scale
	/ s 15
	* s 2.5
	+ sx s
	+ sy s

	image_draw_rotated_scaled img 255 255 255 255 32 32 x y angle sx sy 0 0
end

function logic
start
	= moving 0

	include "poll_joystick.inc"

	var number abs_x
	abs abs_x joy_x1
	var number abs_y
	abs abs_y joy_y1

	? abs_x 0.1
	jge is_moving
	? abs_y 0.1
	jge is_moving
	goto not_moving
:is_moving
	= moving 1

	var number xx
	= xx joy_x1
	* xx 10
	var number yy
	= yy joy_y1
	* yy 10

	+ x xx
	+ y yy

	? x 32
	jge check_right
	= x 32
:check_right
	? x 628
	jle check_top
	= x 628
:check_top
	? y 32
	jge check_bottom
	= y 32
:check_bottom
	? y 288
	jle do_angle
	= y 288

:do_angle
	atan2 angle joy_y1 joy_x1
	var number pi
	= pi 3.14159
	/ pi 2
	+ angle pi

	+ frame_timer 1
	? frame_timer 5
	jl next1
	= frame_timer 0
	+ frame 1
	% frame 3

:next1

	+ scale scale_inc
	? scale_inc 1
	jne dec

	? scale 15
	jne next2
	neg scale_inc

:dec
	? scale 0
	jne next2
	neg scale_inc

:next2
:not_moving

:finish

	include "slideshow_logic.inc"
end

function shutdown
start
	cfg_set_number "x" x
	cfg_set_number "y" y
	cfg_set_number "angle" angle
	cfg_set_number "scale" scale

	cfg_save cfg_name
end
