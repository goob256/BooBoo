var string reset_game_name
= reset_game_name "memory.boo"
include "slideshow_start.inc"

var number r
= r 128
var number g
= g 128
var number b
= b 128

function logic
start
	include "poll_joystick.inc"

	= r joy_x1
	+ r 1
	/ r 2
	* r 255

	= g joy_y1
	+ g 1
	/ g 2
	* g 255

	= b joy_x2
	+ b 1
	/ b 2
	* b 255

label finish
	include "slideshow_logic.inc"
end

function draw
start
	clear r g b
end
