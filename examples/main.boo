include "slideshow_start.inc"

vector programs
vector_add programs "blip.boo"
vector_add programs "clock.boo"
vector_add programs "gameover.boo"
vector_add programs "pong.boo"
vector_add programs "robots.boo"
vector_add programs "secret.boo"
vector_add programs "sine.boo"
vector_add programs "sneaky.boo"
vector_add programs "walk.boo"
number size
vector_size programs size

number sel
= sel 0

number font
font_load font "font.ttf" 16 0
number font_size
font_height font font_size

number del
= del 0

function draw_menu
{
	number y
	= y 10

	number i
	= i 0

:loop
	? i sel
	jne not_selected
	filled_rectangle 255 0 255 255 255 0 255 255 255 0 255 255 255 0 255 255 0 y 640 font_size
:not_selected
	string text
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
	number yy
	= yy y
	+ yy 2
	number sz
	= sz font_size
	- sz 4
	rectangle 255 255 0 255 22 yy sz sz 1
	? __do_slideshow__ 0
	je skip
	number x1
	= x1 22
	number y1
	= y1 yy
	number x2
	= x2 x1
	+ x2 sz
	number y2
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

	number nj
	joystick_count nj
	string s
	string_format s "% Joystick(s) Connected" nj
	number y
	= y 360
	number h
	font_height font h
	- y h
	- y 10
	font_draw font 255 255 255 255 s 10 y
}

function run
{
	? del 0
	je do_joy
	- del 1
	goto finish

:do_joy
	include "poll_joystick.inc"

	? joy_u 0
	je no_u

	- sel 1
	= del 10
	? sel 0
	jge end_up_down
	number sz
	vector_size programs sz
	= sel sz
	goto end_up_down

:no_u

	? joy_d 0
	je no_d

	+ sel 1
	= del 10
	number sz
	vector_size programs sz
	? sel sz
	jle no_d
	= sel 0

:no_d

:end_up_down

	? joy_a 0
	jne do_run
:check_b
	? joy_b 0
	je finish

:do_run
	number sz
	vector_size programs sz
	? sel sz
	jl run_program
	goto toggle

:run_program
	string program
	vector_get programs program sel
	reset program
	goto finish

:toggle
	? __do_slideshow__ 0
	je set_1
	= __do_slideshow__ 0
	= del 10
	goto finish
:set_1
	= __do_slideshow__ 1
	= del 10
	goto finish

:finish
}

function end
{
	cfg_set_number "enabled" __do_slideshow__
	cfg_set_number "delay" __slideshow_delay__
	cfg_save "com.b1stable.slideshow"
}
