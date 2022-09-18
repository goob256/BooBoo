var string reset_game_name
= reset_game_name "blip.boo"
include "slideshow_start.inc"

function logic
start
	include "slideshow_logic.inc"
end

function draw
start
	filled_triangle 255 0 0 255 0 255 0 255 0 0 255 255 0 0 0 360 640 360
	filled_triangle 255 0 0 255 0 0 255 255 0 0 0 255 0 0 640 360 640 0
end
