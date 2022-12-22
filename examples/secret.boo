number music
mml_load music "music/secret.mml"
mml_play music 1.0 1

string reset_game_name
= reset_game_name "sine.boo"
include "slideshow_start.inc"

number hidden
= hidden 1

number font
font_load font "font.ttf" 128 1

function run
{
	include "poll_joystick.inc"

	? joy_a 0
	jne show
	= hidden 1
	goto done_logic

:show
	= hidden 0

:done_logic
	include "slideshow_logic.inc"
}

function draw
{
	clear 255 0 0

	string secret
	? hidden 0
	jne shhh
	= secret "BOOBOO";
	goto done_secret
:shhh
	= secret "******";
:done_secret

	number x
	number y
	number tw
	number fh

	font_width font tw secret
	font_height font fh

	/ tw 2
	/ fh 2

	= x 320
	- x tw

	= y 180
	- y fh

	font_draw font 0 0 0 255 secret x y
}

function end
{
	mml_stop music
}
