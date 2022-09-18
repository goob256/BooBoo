var number music
mml_load music "music/secret.mml"
mml_play music 1.0 1

var string reset_game_name
= reset_game_name "sine.boo"
include "slideshow_start.inc"

var number hidden
= hidden 1

var number font
font_load font "DejaVuSans.ttf" 128 1

function logic
start
	include "poll_joystick.inc"

	? joy_x 0
	jne show
	= hidden 1
	goto done_logic

:show
	= hidden 0

:done_logic
	include "slideshow_logic.inc"
end

function draw
start
	clear 255 0 0

	var string secret
	? hidden 0
	jne shhh
	= secret "BOOBOO";
	goto done_secret
:shhh
	= secret "******";
:done_secret

	var number x
	var number y
	var number tw
	var number fh

	font_width font tw secret
	font_height font fh

	/ tw 2
	/ fh 2

	= x 320
	- x tw

	= y 180
	- y fh

	font_draw font 0 0 0 255 secret x y
end

function shutdown
start
	mml_stop music
end
