number grass
image_load grass "misc/grass.tga"
number robot
image_load robot "misc/robot.tga"
number img_w
number img_h
image_size grass img_w img_h

function draw
{
	number w
	= w 640
	/ w img_w
	number h
	= h 360
	/ h img_h

	image_start grass

	number y
	= y 0

:loop_y
	number x
	= x 0
:loop_x
	number xx
	= xx x
	* xx img_w
	number yy
	= yy y
	* yy img_h
	image_draw grass 255 255 255 255 xx yy 0 0
	+ x 1
	? x w
	jl loop_x
	+ y 1
	? y h
	jl loop_y

	image_end grass

	= x 0
	- w 1
	- h 1
:loop_robot
	number xx
	rand xx 0 w
	* xx img_w
	number yy
	rand yy 0 h
	* yy img_h
	image_draw robot 255 255 255 255 xx yy 0 0
	+ x 1
	? x 17
	jl loop_robot
}
