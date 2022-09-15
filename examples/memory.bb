var number x
var number y

var string cfg_name
= cfg_name "com.b1stable.memory"

var number found
cfg_load found cfg_name
? found 0
jne cfg_found
= x 320
= y 180
goto done_cfg
label cfg_found
cfg_get_number x "x"
cfg_get_number y "y"
label done_cfg

var number radius
= radius 64

function draw
start
	clear 255 0 255

	filled_circle 255 255 0 255 x y radius
end

function logic
start
	var number joy_x1
	var number joy_y1
	var number joy_x2
	var number joy_y2
	var number joy_l
	var number joy_r
	var number joy_u
	var number joy_d
	var number joy_a
	var number joy_b
	var number joy_x
	var number joy_y
	var number joy_lb
	var number joy_rb
	var number joy_back
	var number joy_start

	poll_joystick 0 joy_x1 joy_y1 joy_x2 joy_y2 joy_l joy_r joy_u joy_d joy_a joy_b joy_x joy_y joy_lb joy_rb joy_back joy_start

	? joy_x1 0.1
	jg x_ok
	? joy_x1 -0.1
	jl x_ok
	= joy_x1 0
label x_ok

	? joy_y1 0.1
	jg y_ok
	? joy_y1 -0.1
	jl y_ok
	= joy_y1 0
label y_ok

	* joy_x1 5
	* joy_y1 5
	+ x joy_x1
	+ y joy_y1

	? x radius
	jge check_right
	= x radius

label check_right
	var number right
	= right 640
	- right radius
	? x right
	jle check_top
	= x right

label check_top
	? y radius
	jge check_bottom
	= y radius

label check_bottom
	var number bottom
	= bottom 360
	- bottom radius
	? y bottom
	jle finish
	= y bottom

label finish

	cfg_set_number "x" x
	cfg_set_number "y" y
end

function shutdown
start
	cfg_save cfg_name
end
