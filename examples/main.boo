include "slideshow_start.inc"

var vector programs
vector_add programs "blip.boo"
vector_add programs "clock.boo"
vector_add programs "gameover.boo"
vector_add programs "pong.boo"
vector_add programs "robots.boo"
vector_add programs "secret.boo"
vector_add programs "sine.boo"
vector_add programs "sneaky.boo"
vector_add programs "walk.boo"
var number size
vector_size programs size

var number sel
= sel 0

var number font
font_load font "DejaVuSans.ttf" 16 0
var number font_size
font_height font font_size

var number delay
= delay 0

function draw_menu
{
	var number y
	= y 10

	var number i
	= i 0

:loop
	? i sel
	jne not_selected
	filled_rectangle 255 0 255 255 255 0 255 255 255 0 255 255 255 0 255 255 0 y 640 font_size
:not_selected
	var string text
	vector_get programs text i
	font_draw font 255 255 0 255 text 20 y
	+ y font_size
	+ i 1
	? i size
	jl loop

	? sel i
	jne no_sel2
	filled_rectangle 255 0 255 255 255 0 255 255 255 0 255 255 255 0 255 255 0 y 640 font_size

:no_sel2
	var number yy
	= yy y
	+ yy 2
	var number sz
	= sz font_size
	- sz 4
	rectangle 255 255 0 255 22 yy sz sz 1
	? __do_slideshow__ 0
	je skip
	var number x1
	= x1 22
	var number y1
	= y1 yy
	var number x2
	= x2 x1
	+ x2 sz
	var number y2
	= y2 y1
	+ y2 sz
	line 255 255 0 255 x1 y1 x2 y2 1
	line 255 255 0 255 x1 y2 x2 y1 1

:skip
	font_draw font 255 255 0 255 "ENABLE SLIDESHOW" 40 y
}

function draw
{
	clear 0 0 0

	call draw_menu

	var number nj
	joystick_count nj
	var string s
	string_format s "% Joystick(s) Connected" nj
	var number y
	= y 360
	var number h
	font_height font h
	- y h
	- y 10
	font_draw font 255 255 255 255 s 10 y
}

function run
{
	? delay 0
	je do_joy
	- delay 1
	goto finish

:do_joy
	include "poll_joystick.inc"

	? joy_u 0
	je no_u

	- sel 1
	= delay 10
	? sel 0
	jge end_up_down
	var number sz
	vector_size programs sz
	= sel sz
	goto end_up_down

:no_u

	? joy_d 0
	je no_d

	+ sel 1
	= delay 10
	var number sz
	vector_size programs sz
	? sel sz
	jle no_d
	= sel 0

:no_d

:end_up_down

	? joy_a 0
	je finish

	var number sz
	vector_size programs sz
	? sel sz
	jl run_program
	goto toggle

:run_program
	var string program
	vector_get programs program sel
	reset program
	goto finish

:toggle
	? __do_slideshow__ 0
	je set_1
	= __do_slideshow__ 0
	= delay 10
	goto finish
:set_1
	= __do_slideshow__ 1
	= delay 10
	goto finish

:finish
}

function end
{
	cfg_set_number "enabled" __do_slideshow__
	cfg_set_number "delay" __slideshow_delay__
	cfg_save "com.b1stable.slideshow"
}
