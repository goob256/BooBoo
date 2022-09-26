var string reset_game_name
= reset_game_name "blip.boo"
include "slideshow_start.inc"

function run
start
	include "slideshow_logic.inc"
end

function draw
start
	filled_rectangle 255 0 0 255 0 255 0 255 0 0 255 255 0 0 0 255 0 0 640 360
end
