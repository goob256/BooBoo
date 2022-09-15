; subroutines

var vector params
vector_add params 1

var number font
load_font font "DejaVuSans.ttf" 128

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

	? x 0
	jne show
	vector_set params 0 1
	goto done_logic

label show
	vector_set params 0 0

label done_logic
end

function draw
start
	clear 255 0 0

	var string secret
	sub secret "subroutine.bb" params

	var number x
	var number y
	var number tw
	var number fh

	text_width font tw secret
	font_height font fh

	/ tw 2
	/ fh 2

	= x 320
	- x tw

	= y 180
	- y fh

	draw_text font 0 0 0 255 secret x y
end
