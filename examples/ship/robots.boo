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
var number old_l
var number old_r
var number old_u
var number old_d
= old_l 0
= old_r 0
= old_u 0
= old_d 0
var number tick
= tick 0
var number gameover
= gameover 0
var number gameover_start
var number robot_img
var number dog_img
var number fire_img
var number grass_img
load_image robot_img "misc/robot.tga"
load_image dog_img "misc/dog.tga"
load_image fire_img "misc/explosion.tga"
load_image grass_img "misc/grass.tga"

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

;	var number xxx
;	var number yyy
;	= xxx xx
;	- xxx 0.5
;	= yyy yy
;	- yyy 0.5
;	var number w
;	= w rect_w
;	+ w 1
;	var number h
;	= h rect_h
;	+ h 1
;	rectangle 0 0 255 255 xxx yyy w h 2

	draw_image grass_img xx yy

	? value 1
	jl past_draw

	var number image

	? value 1
	je player
	? value 2
	je robot
	? value 3
	je crash

	goto past_draw

label player
	= image dog_img
	goto done_image

label robot
	= image robot_img
	goto done_image
label crash
	= image fire_img

label done_image

;	var number w
;	= w rect_w
;	* w 0.75
;	var number h
;	= h rect_h
;	* h 0.75
;	var number dx
;	var number dy
;	= dx rect_w
;	- dx w
;	= dy rect_h
;	- dy h
;	/ dx 2
;	/ dy 2
;	var number xxx
;	var number yyy
;	= xxx xx
;	+ xxx dx
;	= yyy yy
;	+ yyy dy
;
;	filled_rectangle red green blue 255 red green blue 255 red green blue 255 red green blue 255 xxx yyy w h
;
	draw_image image xx yy

label past_draw

	+ x 1
	? x width
	jl loop_x
	
	+ y 1
	? y height
	jl loop_y
end

function move_robots
start
	var number i
	var number size
	vector_size board size
	var number x
	= x 0
	var number y
	= y 0

	= i 0
label top

	var number value
	call = value get_board board x y

	? value 2
	jne continue2

	var number dx
	var number dy
	var number dir_x
	var number dir_y
	var number dx_abs
	var number dy_abs

	= dx x
	- dx player_x
	= dy y
	- dy player_y
	= dx_abs dx
	= dy_abs dy

	? dx 0
	je done_x
	jl x_less
	= dir_x -1
	goto done_x

label x_less
	neg dx_abs
	= dir_x 1

label done_x

	? dy 0
	je done_y
	jl y_less
	= dir_y -1
	goto done_y

label y_less
	neg dy_abs
	= dir_y 1

label done_y

	var number new_x
	= new_x x
	var number new_y
	= new_y y

	? dx_abs dy_abs
	jl move_y
	+ new_x dir_x
	goto done_move

label move_y

	+ new_y dir_y

label done_move ; not really

	call set_board board x y 0

	var number value
	call = value get_board board new_x new_y

	? value 0
	jne crash

	call set_board board new_x new_y 2

	goto continue2

label crash
	call set_board board new_x new_y 3
	? value 1
	je game_over
	goto continue2

label game_over
	? gameover 1
	je dont_set
	= gameover 1
	= gameover_start tick
label dont_set

label continue2

	+ x 1
	? x width
	jne continue
	= x 0
	+ y 1
label continue
	+ i 1
	? i size
	jl top
end

function move_player l r u d
start
	var number index
	= index player_y
	* index width
	+ index player_x

	? l 0
	je next1

	- player_x 1
	? player_x 0
	jge finish
	= player_x 0
	goto finish

label next1

	? r 0
	je next2

	+ player_x 1
	? player_x width
	jl finish
	= player_x width
	- player_x 1
	goto finish


label next2

	? u 0
	je next3

	- player_y 1
	? player_y 0
	jge finish
	= player_y 0
	goto finish



label next3

	? d 0
	je finish

	+ player_y 1
	? player_y height
	jl finish
	= player_y height
	- player_y 1
	goto finish

label finish

	vector_set board index 0

	= index player_y
	* index width
	+ index player_x

	call move_robots

	var number value
	call = value get_board board player_x player_y

	? value 2
	jl done

	; gameover
	= gameover 1
	= gameover_start tick
	vector_set board index 3

label done
	? gameover 0
	jne really_done

	vector_set board index 1

label really_done
end

function logic
start
	var number sz
	vector_size board sz
	var number i
	var number count
	= count 0
	= i 0
label loop_top
	var number value
	vector_get board value i
	? value 2
	jne another
	+ count 1
label another
	+ i 1
	? i sz
	jl loop_top

	? count 0
	jne do_input
	? gameover 1
	je do_input
	= gameover 1
	= gameover_start tick

label do_input
	+ tick 1

	include "poll_joystick.inc"

	var number bak_l
	var number bak_r
	var number bak_u
	var number bak_d
	= bak_l joy_l
	= bak_r joy_r
	= bak_u joy_u
	= bak_d joy_d


	? joy_l 0
	je next_dir1
	? old_l 0
	jne zero_l
	goto next_dir1
label zero_l
	= joy_l 0
label next_dir1

	? joy_r 0
	je next_dir2
	? old_r 0
	jne zero_r
	goto next_dir2
label zero_r
	= joy_r 0
label next_dir2

	? joy_u 0
	je next_dir3
	? old_u 0
	jne zero_u
	goto next_dir3
label zero_u
	= joy_u 0
label next_dir3

	? joy_d 0
	je next_dir4
	? old_d 0
	jne zero_d
	goto next_dir4
label zero_d
	= joy_d 0
label next_dir4

	? joy_l 0
	jne move
	? joy_r 0
	jne move
	? joy_u 0
	jne move
	? joy_d 0
	jne move
	goto finish

label move
	call move_player joy_l joy_r joy_u joy_d

label finish
	= old_l bak_l
	= old_r bak_r
	= old_u bak_u
	= old_d bak_d
end
