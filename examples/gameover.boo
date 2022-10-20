string reset_game_name
= reset_game_name "pong.boo"
include "slideshow_start.inc"

number music
mml_load music "music/gameover.mml"
mml_play music 1.0 1

number guy
number glasses

image_load guy "misc/guy.png"
image_load glasses "misc/glasses.png"

number y
= y 0

function run
{
	+ y 2.5
	include "slideshow_logic.inc"
}

function draw
{
	image_draw guy 255 255 255 255 0 0 0 0

	number yy
	= yy y
	% yy 180
	- yy 100

	image_draw glasses 255 255 255 255 180 yy 0 0
}

function end
{
	mml_stop music
}
