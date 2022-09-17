; joystick input and sfx

var string reset_game_name
= reset_game_name "gameover.boo"
include "slideshow_start.inc"

var number radius
= radius 5000

var number blip
create_mml blip "@PO0 = { 0 100 100 100 0 }\n@VAS0 = { 0 }\nA o5 @TYPE3 @PO0 @VAS0 a16 @VAS0 @PO0"

var number was_pressed
= was_pressed 0

function logic
start
	include "poll_joystick.inc"

	? joy_a 1
	je currently_pressed

	= was_pressed 0
	goto finish

:currently_pressed
	? was_pressed 0
	je go
	goto finish

:go
	= was_pressed 1
	= radius 0
	play_mml blip

:finish
	? radius 5000
	jge really_finish
	+ radius 25
:really_finish
	include "slideshow_logic.inc"
end

function draw
start
	clear 0 0 0
	circle 0 255 0 255 320 180 radius 1 -1
end
