function rand_x
start
	var number x
	rand x 0 640
	return x
end

function rand_y
start
	var number y
	rand y 0 360
	return y
end

function draw_a_line x1 y1 x2 y2 thickness
start
	line 0 0 0 255 x1 y1 x2 y2 thickness
end

function draw
start
	clear 0 216 255

	var number i

	start_primitives

	= i 20
label repeat_x
	line 255 216 0 255 i 0 i 360 1
	+ i 20
	compare i 640
	jne repeat_x

	= i 20
label repeat_y
	line 255 216 0 255 0 i 640 i 1
	+ i 20
	compare i 360
	jne repeat_y

	call draw_a_line 0 0 640 360 1
	call draw_a_line 0 360 640 0 1

	filled_triangle 255 0 0 255 0 255 0 255 0 0 255 255 0 0 640 0 320 360

	filled_rectangle 255 255 255 255 255 255 255 255 0 0 0 255 0 0 0 255 50 50 300 300

	var number rx
	var number rx2
	var number ry
	var number ry2

	call = rx rand_x
	call = rx2 rand_x
	call = ry rand_y
	call = ry2 rand_y

	line 255 0 0 255 rx ry rx2 ry2 16

	rectangle 128 128 128 255 0 0 640 360 16

	end_primitives
end
