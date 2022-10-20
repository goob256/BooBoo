string reset_game_name
= reset_game_name "clock.boo"
include "slideshow_start.inc"

number size
= size 8

vector fonts

:font_loads

number f

font_load f "font.ttf" size 0

vector_add fonts f

+ size 8

? size 64
jle font_loads

number half_x
= half_x 320

string text
= text "THE SNEAKY LITTLE CAT"

number loops
= loops 0

function draw
{
	clear 255 255 255

	number l
	= l loops
	/ l 5
	% l 9

	? l 0
	je no_draw

	number y
	= y 16

	number x
	number w

	number i
	= i 0
:draw_line
	= x half_x
	number f
	vector_get fonts f i
	font_width f w text
	/ w 2
	- x w
	font_draw f 0 0 0 255 text x y
	number fh
	font_height f fh
	+ y fh
	+ y 5
	+ i 1
	? i l
	jl draw_line

:no_draw
}

function run
{
	+ loops 1
	include "slideshow_logic.inc"
}
