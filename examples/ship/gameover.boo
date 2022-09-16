; music and images

var string reset_game_name
= reset_game_name "memory.boo"
include "slideshow_start.inc"

play_music "music/gameover.mml"

var number guy
var number glasses

load_image guy "misc/guy.png"
load_image glasses "misc/glasses.png"

var number y
= y 0

function logic
start
	+ y 2.5
	include "slideshow_logic.inc"
end

function draw
start
	draw_image guy 0 0

	var number yy
	= yy y
	% yy 180
	- yy 100

	draw_image glasses 180 yy
end

function shutdown
start
	stop_music
end
