play_music "music/gameover.mml"

var number guy
var number glasses

load_image guy "misc/guy.tga"
load_image glasses "misc/glasses.tga"

var number y
= y 0

function logic
start
	+ y 2.5
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
