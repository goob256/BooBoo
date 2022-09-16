var number start_sz
= start_sz 20
var number sz
= sz start_sz

var number w
var number h

var vector colours
var number j
= j 0
label fill_colours
var number n
= n j
* n 2
rand n 0 255
vector_add colours n
+ j 1
? j 128
jl fill_colours
= j 0
label fill_colours2
var number n
= n j
/ n 128
var number n2
= n2 1
- n2 n
* n2 255
rand n2 0 255
vector_add colours2 n2
+ j 1
? j 128
jl fill_colours2

function generate sz
start
	= h 0

	var vector v

	var number y
	= y 0

label loop_y
	var number x
	= x 0
	= w 0
label loop_x
	var number r
	rand r 0 2
	var number index
	= index 255
	/ index 4
	* index r
	% index 256

	vector_add v index

	+ w 1
	+ x sz
	? x 640
	jl loop_x

	+ h 1
	+ y sz
	? y 360
	jl loop_y

	return v
end

function safe_set v index val
start
	? index 0
	jl done
	var number sz
	vector_size v sz
	? index sz
	jge done
	vector_set v index val
label done
end

function safe_get v x y
start
	var number index
	= index y
	* index w
	+ index x

	? index 0
	jl zero

	var number sz
	vector_size v sz
	? index sz
	jge zero

	var number val
	vector_get v val index

	return val

label zero
	return 0
end

function average v x y
start
	var number tot
	= tot 0
	var number val

	var number xx
	= xx x
	var number yy
	= yy y
	- xx 0.5

	call = val safe_get v xx yy
	+ tot val

	= xx x
	= yy y
	- yy 0.5

	call = val safe_get v xx yy
	+ tot val

	= xx x
	= yy y
	+ xx 0.5

	call = val safe_get v xx yy
	+ tot val

	= xx x
	= yy y
	+ yy 0.5

	call = val safe_get v xx yy
	+ tot val

	/ tot 4
	return tot
end

function break_down v
start
	var vector double

	* w 2
	* h 2

	var number new_count
	= new_count w
	* new_count h

	var number i
	= i 0
label fill_top
	vector_add double 0
	+ i 1
	? i new_count
	jl fill_top

	var number y
	= y 0

label loop_y
	var number x
	= x 0
label loop_x
	var number xx
	= xx x
	+ xx 0.25
	var number yy
	= yy y
	+ yy 0.25

	var number new_val

	call = new_val average v xx yy

	var number new_x
	= new_x x
	* new_x 2
	var number new_y
	= new_y y
	* new_y 2
	
	var number index
	
	= index new_x
	var number i2
	= i2 new_y
	* i2 w
	+ index i2
	call safe_set double index new_val

	+ xx 0.5

	call = new_val average v xx yy

	+ new_x 1

	= index new_x
	var number i2
	= i2 new_y
	* i2 w
	+ index i2
	call safe_set double index new_val

	- xx 0.0
	+ yy 0.5

	call = new_val average v xx yy

	- new_x 1
	+ new_y 1

	= index new_x
	var number i2
	= i2 new_y
	* i2 w
	+ index i2
	call safe_set double index new_val

	+ xx 0.5

	call = new_val average v xx yy

	+ new_x 1

	= index new_x
	var number i2
	= i2 new_y
	* i2 w
	+ index i2
	call safe_set double index new_val

	+ x 1
	? x w
	jl loop_x

	+ y 1
	? y h
	jl loop_y
	
	return double
end

var vector v
call = v generate sz

var vector result

var number iterations
= iterations 4
var number it
= it 0

label iteration
call = result break_down v
= v result
+ it 1
? it iterations
jl iteration

function draw
start
	clear 255 0 0

	var number i
	= i 0

	var number y
	= y 0

label loop_y
	var number x
	= x 0
label loop_x
	var number val
	vector_get v val i
	var number val2
	vector_get colours val2 val
	filled_rectangle 0 0 val2 255 0 0 val2 255 0 0 val2 255 0 0 val2 255 x y sz sz
	+ i 1
	+ x sz
	? x 640
	jl loop_x

	+ y sz
	? y 360
	jl loop_y
end
