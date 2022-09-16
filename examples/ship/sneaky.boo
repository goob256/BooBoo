; vectors and fonts

var string reset_game_name
= reset_game_name "clock.boo"
include "slideshow_start.inc"

var number size
= size 8

var vector fonts

label load_fonts

var number f

load_font f "DejaVuSans.ttf" size

vector_add fonts f

+ size 8

? size 64
jle load_fonts

var number i
= i 0
label smooth
var number f
vector_get fonts f i
font_smooth f 0
+ i 1
var number sz
vector_size fonts sz
? i sz
jl smooth

var number half_x
= half_x 320

var string text
= text "THE SNEAKY LITTLE CAT"

var number loops
= loops 0

function draw
start
	clear 255 255 255

	var number l
	= l loops
	/ l 5
	% l 9

	? l 0
	je no_draw

	var number y
	= y 16

	var number x
	var number w

	var number i
	= i 0
label draw_line
	= x half_x
	var number f
	vector_get fonts f i
	text_width f w text
	/ w 2
	- x w
	draw_text f 0 0 0 255 text x y
	var number fh
	font_height f fh
	+ y fh
	+ y 5
	+ i 1
	? i l
	jl draw_line

label no_draw
end

function logic
start
	+ loops 1
	include "slideshow_logic.inc"
end
