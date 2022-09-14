var number radius
= radius 10000

var number blip
load_mml blip "sfx/bigshot.mml"

var number was_pressed
= was_pressed 0

function logic
start
	var number x1
	var number y1
	var number x2
	var number y2
	var number l
	var number r
	var number u
	var number d
	var number a
	var number b
	var number x
	var number y
	var number lb
	var number rb
	var number back
	var number start

	poll_joystick 0 x1 y1 x2 y2 l r u d a b x y lb rb back start

	? a 1
	je currently_pressed

	= was_pressed 0
	goto finish

label currently_pressed
	? was_pressed 0
	je go
	goto finish

label go
	= was_pressed 1
	= radius 0
	play_mml blip

label finish
	+ radius 25
end

function draw
start
	clear 0 0 0
	circle 0 255 0 255 320 180 radius 1
end
