; math and circles

function draw_at r g b x radius
start
	var number f
	= f x
	/ f 640
	* f 3.14159
	* f 2
	sin f f
	* f 90
	+ f 180
	filled_circle r g b 255 x f radius
end

function draw
start
	clear 0 0 0

	var number xx
	= xx 0

	start_primitives

label loop
	call draw_at 0 255 0 xx 8
	+ xx 1
	? xx 640
	jl loop

	var number tmp
	= tmp x
	% tmp 640

	call draw_at 128 255 128 tmp 32

	end_primitives
end

function logic
start
	+ x 5
end

var number x
= x 0