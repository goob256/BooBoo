string reset_game_name
= reset_game_name "robots.boo"
include "slideshow_start.inc"

function draw_at r g b x radius
{
	number f
	= f x
	/ f 640
	* f 3.14159
	* f 2
	sin f f
	* f 90
	+ f 180
	filled_circle r g b 255 x f radius -1
}

function draw
{
	clear 0 0 0

	number xx
	= xx 0

	start_primitives

:loop
	call draw_at 0 255 0 xx 8
	+ xx 2
	? xx 640
	jl loop

	number tmp
	= tmp x
	% tmp 640

	call draw_at 128 255 128 tmp 32

	end_primitives
}

function run
{
	+ x 5
	include "slideshow_logic.inc"
}

number x
= x 0
