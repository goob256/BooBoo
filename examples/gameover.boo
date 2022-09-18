var string reset_game_name
= reset_game_name "pong.boo"
include "slideshow_start.inc"

var number music
mml_load music "music/gameover.mml"
mml_play music 1.0 1

var number guy
var number glasses

image_load guy "misc/guy.png"
image_load glasses "misc/glasses.png"

var number y
= y 0

function logic
start
	+ y 2.5
	include "slideshow_logic.inc"
end

function draw
start
	image_draw guy 255 255 255 255 0 0 0 0

	var number yy
	= yy y
	% yy 180
	- yy 100

	image_draw glasses 255 255 255 255 180 yy 0 0
end

function shutdown
start
	mml_stop music
end
