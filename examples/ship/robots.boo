var number width
= width 32
var number height
= height 18
var number rect_w
= rect_w 640
/ rect_w width
var number rect_h
= rect_h 360
/ rect_h height
var number num_robots
= num_robots 16
var number player_x
var number player_y

function set_board board x y value
start
	var number index
	= index y
	* index width
	+ index x
	? index 0
	jl skip
	var number size
	vector_size board size
	? index size
	jge skip

	vector_set board index value

label skip
end

function get_board board x y
start
	var number index
	= index y
	* index width
	+ index x
	? index 0
	jl skip
	var number size
	vector_size board size
	? index size
	jge skip

	var number ret
	vector_get board ret index
	return ret

label skip
	return 0
end

function start_board
start
	var vector board

	var number count
	= count width
	* count height

	var number i
	= i 0
label fill_board
	vector_add board 0
	+ i 1
	? i count
	jl fill_board

	; position player

	var number x
	var number y
	var number w
	= w width
	- w 1
	var number h
	= h height
	- h 1
	rand x 0 w
	rand y 0 h

	call set_board board x y 1
	= player_x x
	= player_y y

	= i 0
label place_robot
	rand x 0 w
	rand y 0 h
	var number value
	call = value get_board board x y
	? value 0
	jne place_robot
	; stay a little distance from the player
	var number dx
	var number dy
	= dx player_x
	- dx x
	= dy player_y
	- dy y
	* dx dx
	* dy dy
	+ dx dy
	sqrt dx dx
	; dx is now distance from player
	? dx 8
	jl place_robot
	call set_board board x y 2
	+ i 1
	? i num_robots
	jl place_robot

	return board
end

var vector board
call = board start_board

function draw
start
	clear 255 255 255

	var number x
	var number y

	var number xx
	var number yy

	= y 0
label loop_y
	= x 0
label loop_x
	var number value
	call = value get_board board x y

	= xx x
	* xx rect_w
	= yy y
	* yy rect_h

	var number xxx
	var number yyy
	= xxx xx
	- xxx 0.5
	= yyy yy
	- yyy 0.5
	var number w
	= w rect_w
	+ w 1
	var number h
	= h rect_h
	+ h 1
	rectangle 0 0 255 255 xxx yyy w h 2

	? value 1
	jl past_draw

	var number red
	var number green
	var number blue

	? value 1
	je player
	? value 2
	je robot

	goto past_draw

label player
	= red 0
	= green 255
	= blue 0
	goto done_colour

label robot
	= red 128
	= green 128
	= blue 128

label done_colour

	var number w
	= w rect_w
	* w 0.75
	var number h
	= h rect_h
	* h 0.75
	var number dx
	var number dy
	= dx rect_w
	- dx w
	= dy rect_h
	- dy h
	/ dx 2
	/ dy 2
	var number xxx
	var number yyy
	= xxx xx
	+ xxx dx
	= yyy yy
	+ yyy dy

	filled_rectangle red green blue 255 red green blue 255 red green blue 255 red green blue 255 xxx yyy w h

label past_draw

	+ x 1
	? x width
	jl loop_x
	
	+ y 1
	? y height
	jl loop_y
end
