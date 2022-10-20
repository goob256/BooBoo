number crash_mml
mml_create crash_mml "A @TYPE1 a16"

string reset_game_name
= reset_game_name "walk.boo"
include "slideshow_start.inc"

number width
= width 32
number height
= height 18
number rect_w
= rect_w 640
/ rect_w width
number rect_h
= rect_h 360
/ rect_h height
number num_robots
= num_robots 16
number player_x
number player_y
number old_l
number old_r
number old_u
number old_d
= old_l 0
= old_r 0
= old_u 0
= old_d 0
number tick
= tick 0
number next_cpu_move
= next_cpu_move 60
number gameover
= gameover 0
number gameover_start
number robot_img
number dog_img
number fire_img
number grass_img
image_load robot_img "misc/robot.tga"
image_load dog_img "misc/dog.tga"
image_load fire_img "misc/explosion.tga"
image_load grass_img "misc/grass.tga"

vector board
call start_board

function set_board x y value
{
	number index
	= index y
	* index width
	+ index x
	? index 0
	jl skip1
	number size
	vector_size board size
	? index size
	jge skip1

	vector_set board index value

:skip1
}

function get_board x y
{
	number index
	= index y
	* index width
	+ index x
	? index 0
	jl skip2
	number size
	vector_size board size
	? index size
	jge skip2

	number ret
	vector_get board ret index
	return ret

:skip2
	return 0
}

function start_board
{
	vector_clear board

	number count
	= count width
	* count height

	number i
	= i 0
:fill_board
	vector_add board 0
	+ i 1
	? i count
	jl fill_board

	; position player

	number x
	number y
	number w
	= w width
	- w 1
	number h
	= h height
	- h 1
	rand x 0 w
	rand y 0 h

	call set_board x y 1
	= player_x x
	= player_y y

	= i 0
:place_robot
	rand x 0 w
	rand y 0 h
	number value
	call_result value get_board x y
	? value 0
	jne place_robot
	; stay a little distance from the player
	number dx
	number dy
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
	call set_board x y 2
	+ i 1
	? i num_robots
	jl place_robot
}

function draw
{
	number x
	number y

	number xx
	number yy

	image_start grass_img

	number index
	= index 0

	= y 0
:loop_y
	= x 0
:loop_x

	= xx x
	* xx rect_w
	= yy y
	* yy rect_h

	image_draw grass_img 255 255 255 255 xx yy 0 0

	+ x 1
	? x width
	jl loop_x
	
	+ y 1
	? y height
	jl loop_y

	image_end grass_img

	= y 0
:loop_y1
	= x 0
:loop_x1
	number value
	vector_get board value index

	= xx x
	* xx rect_w
	= yy y
	* yy rect_h

	;image_draw grass_img 255 255 255 255 xx yy 0 0

	number image

	? value 1
	je player
	? value 2
	je robot
	? value 3
	je crash1

	goto past_draw

:player
	= image dog_img
	goto done_image

:robot
	= image robot_img
	goto done_image
:crash1
	= image fire_img

:done_image

	image_draw image 255 255 255 255 xx yy 0 0

:past_draw
	+ index 1

	+ x 1
	? x width
	jl loop_x1
	
	+ y 1
	? y height
	jl loop_y1
}

function move_robots
{
	number size
	vector_size board size

	vector robot_indices
	number nbots
	= nbots 0

	number i
	= i 0
:find_robots
	number value
	vector_get board value i
	? value 2
	jne next_robot
	vector_add robot_indices i
	+ nbots 1
:next_robot
	+ i 1
	? i size
	jl find_robots

	? nbots 0
	jne top3

	= gameover 1
	= gameover_start tick

	goto finish1

:top3
	= i 0
:top1
	number index
	vector_get robot_indices index i

	number x
	= x index
	% x width
	number y
	= y index
	/ y width
	int y

	number value
	vector_get board value index

	? value 2
	jne continue

	number dx
	number dy
	number dir_x
	number dir_y
	number dx_abs
	number dy_abs

	= dir_x 0
	= dir_y 0

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

:x_less
	neg dx_abs
	= dir_x 1

:done_x

	? dy 0
	je done_y
	jl y_less
	= dir_y -1
	goto done_y

:y_less
	neg dy_abs
	= dir_y 1

:done_y

	number new_x
	= new_x x
	number new_y
	= new_y y

	? dx_abs dy_abs
	jl move_y
	+ new_x dir_x
	goto done_move

:move_y

	+ new_y dir_y

:done_move

	vector_set board index 0

	number value
	call_result value get_board new_x new_y

	? value 0
	jne crash2

	call set_board new_x new_y 2

	goto continue

:crash2
	call set_board new_x new_y 3
	mml_play crash_mml 1.0 0
	? value 1
	je game_over
	goto continue

:game_over
	? gameover 1
	je dont_set
	= gameover 1
	= gameover_start tick
:dont_set

:continue
	+ i 1
	? i nbots
	jl top1

:finish1
}

function move_player l r u d
{
	number index
	= index player_y
	* index width
	+ index player_x

	? l 0
	je next1

	- player_x 1
	? player_x 0
	jge finish2
	= player_x 0
	goto finish2

:next1

	? r 0
	je next2

	+ player_x 1
	? player_x width
	jl finish2
	= player_x width
	- player_x 1
	goto finish2


:next2

	? u 0
	je next3

	- player_y 1
	? player_y 0
	jge finish2
	= player_y 0
	goto finish2



:next3

	? d 0
	je finish2

	+ player_y 1
	? player_y height
	jl finish2
	= player_y height
	- player_y 1
	goto finish2

:finish2

	vector_set board index 0

	= index player_y
	* index width
	+ index player_x

	call move_robots

	number value
	call_result value get_board player_x player_y

	? value 2
	jl done

	; gameover
	= gameover 1
	= gameover_start tick
	vector_set board index 3
	mml_play crash_mml 1.0 0

:done
	? gameover 0
	jne really_done

	vector_set board index 1

:really_done
}

function joystick_input
{
:do_input
	include "poll_joystick.inc"

	number bak_l
	number bak_r
	number bak_u
	number bak_d
	= bak_l joy_l
	= bak_r joy_r
	= bak_u joy_u
	= bak_d joy_d
	
:continue_input

	? joy_l 0
	je next_dir1
	? old_l 0
	jne zero_l
	goto next_dir1
:zero_l
	= joy_l 0
:next_dir1

	? joy_r 0
	je next_dir2
	? old_r 0
	jne zero_r
	goto next_dir2
:zero_r
	= joy_r 0
:next_dir2

	? joy_u 0
	je next_dir3
	? old_u 0
	jne zero_u
	goto next_dir3
:zero_u
	= joy_u 0
:next_dir3

	? joy_d 0
	je next_dir4
	? old_d 0
	jne zero_d
	goto next_dir4
:zero_d
	= joy_d 0
:next_dir4

	? joy_l 0
	jne move
	? joy_r 0
	jne move
	? joy_u 0
	jne move
	? joy_d 0
	jne move
	goto finish3

:move
	call move_player joy_l joy_r joy_u joy_d
	
:finish3
	= old_l bak_l
	= old_r bak_r
	= old_u bak_u
	= old_d bak_d
}

function cpu_input
{
	; make vectors of the positions to the four cardinal directions of the player

	number bak_x
	number bak_y
	= bak_x player_x
	= bak_y player_y

	number dx
	number dy

	number x
	number y
	vector vx
	vector vy
	
	= x player_x
	+ x -1

	= y player_y

	vector_add vx x
	vector_add vy y
	
	= x player_x
	+ x 1

	= y player_y

	vector_add vx x
	vector_add vy y
	
	= x player_x

	= y player_y
	- y 1

	vector_add vx x
	vector_add vy y
	
	= x player_x

	= y player_y
	+ y 1

	vector_add vx x
	vector_add vy y

	number i

	number removed

	= i 0
:top2
	= removed 0

	number xx
	number yy

	vector_get vx xx i
	vector_get vy yy i

	? xx 0
	jl remove
	? yy 0
	jl remove
	? xx width
	jge remove
	? yy height
	jge remove

	number value

	call_result value get_board xx yy

	? value 0
	jne remove

	goto end_loop

:remove
	= removed 1
	vector_erase vx i
	vector_erase vy i

:end_loop
	? removed 1
	je no_inc
	+ i 1
:no_inc
	number size
	vector_size vx size
	? i size
	jl top2

	number size
	vector_size vx size

	? size 0
	jne pick_random

	; move toward centre, nothing else to do

	number w
	= w width
	number h
	= h height
	/ w 2
	/ h 2

	? player_x w
	jl right
	= dx -1
	goto do_y
:right
	= dx 1

:do_y

	? player_y h
	jl bottom
	= dy -1
	goto do_move
:bottom
	= dy 1

	goto do_move

:pick_random
	number r
	number size
	vector_size vx size
	- size 1
	rand r 0 size

	number xx
	number yy

	vector_get vx xx r
	vector_get vy yy r

	number l
	number r
	number u
	number d
	= l 0
	= r 0
	= u 0
	= d 0

	? xx player_x
	jl neg_x
	jg pos_x
	goto set_y

:neg_x
	= l 1
	goto set_y
:pos_x
	= r 1
	goto set_y

:set_y

	? yy player_y
	jl neg_y
	jg pos_y
	goto do_move

:neg_y
	= u 1
	goto do_move
:pos_y
	= d 1

:do_move

	call move_player l r u d
}

function run
{
	+ tick 1

	number sz
	vector_size board sz
	number i
	number count
	= count 0
	= i 0

	? gameover 1
	je next

:loop_top
	number value
	vector_get board value i
	? value 2
	jne another
	+ count 1
:another
	+ i 1
	? i sz
	jl loop_top

	? count 0
	jne next
	= gameover 1
	= gameover_start tick

:next
	? gameover 1
	jne lets_go

	number t
	= t tick
	- t gameover_start

	? t 180
	jl finish4

	= gameover 0
	call start_board

:lets_go

	number nj
	joystick_count nj

	? nj 0
	je cpu

	call joystick_input

	goto finish4

:cpu
	? gameover 0
	jne finish4

	? next_cpu_move tick
	jge finish4
	= next_cpu_move tick
	+ next_cpu_move 60
	call cpu_input

:finish4
	include "slideshow_logic.inc"
}
