var number paddle1
var number paddle2
var number ball_x
var number ball_y
var number prev_ball_x
var number prev_ball_y
var number ball_vx
var number ball_vy
var number ball_radius
var number paddle_w
var number paddle_h

= ball_radius 10
= paddle1 180
= paddle2 180
= paddle_w 20
= paddle_h 50

var number half
= half paddle_h
/ half 2

var number wall_sfx
var number point_sfx
load_mml wall_sfx "sfx/wall.mml"
load_mml point_sfx "sfx/point.mml"

function reset_ball
start
	= ball_x 320
	= ball_y 180
	= prev_ball_x 320
	= prev_ball_y 180

	var number r
	rand r 0 1
	? r 0
	je left
	= ball_vx 2.5
	goto done_horiz
label left
	= ball_vx -2.5
label done_horiz

	rand r 0 1
	? r 0
	je up 
	= ball_vy 2.5
	goto done_vert
label up
	= ball_vy -2.5
label done_vert
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

	? joy_y1 0
	jl below
	? joy_y1 0.1
	jl done_joy
	* joy_y1 5.0
	+ paddle1 joy_y1
	var number bot
	= bot 360
	- bot half
	? paddle1 bot
	jl done_joy
	= paddle1 bot

label below
	? joy_y1 -0.1
	jg done_joy
	* joy_y1 5.0
	+ paddle1 joy_y1
	? paddle1 half
	jg done_joy
	= paddle1 half

label done_joy

	var number nj
	num_joysticks nj
	? nj 2
	jl cpu

label player2
	poll_joystick 1 joy_x1 joy_y1 joy_x2 joy_y2 joy_l joy_r joy_u joy_d joy_a joy_b joy_x joy_y joy_lb joy_rb joy_back joy_start

	? joy_y1 0
	jl below2
	? joy_y1 0.1
	jl done_joy2
	* joy_y1 5.0
	+ paddle2 joy_y1
	var number bot
	= bot 360
	- bot half
	? paddle2 bot
	jl done_joy2
	= paddle2 bot

label below2
	? joy_y1 -0.1
	jg done_joy2
	* joy_y1 5.0
	+ paddle2 joy_y1
	? paddle2 half
	jg done_joy2
	= paddle2 half

label done_joy2

	goto done_cpu

label cpu
	? ball_vx 0
	jle done_cpu

	var number diff
	= diff paddle2
	- diff ball_y
	? diff 0
	jge not_neg
	neg diff
label not_neg
	? diff 10
	jle done_cpu

	? paddle2 ball_y
	jl move_up

	- paddle2 5.0
	? paddle2 half
	jge done_cpu
	= paddle2 half
	goto done_cpu

label move_up

	+ paddle2 5.0
	var number bott
	= bott 360
	- bott half
	? paddle2 bott
	jle done_cpu
	= paddle2 bott

label done_cpu

	+ ball_x ball_vx
	? ball_x ball_radius
	jge no_point2

	call reset_ball
	play_mml point_sfx

label no_point2

	var number right
	= right 640
	- right ball_radius
	? ball_x right
	jle no_point1

	call reset_ball
	play_mml point_sfx

label no_point1
	
	+ ball_y ball_vy
	? ball_y ball_radius
	jge no_bounce_top

	- ball_y ball_vy
	neg ball_vy
	play_mml wall_sfx

label no_bounce_top

	var number bottom
	= bottom 360
	- bottom ball_radius
	? ball_y bottom
	jle no_bounce_bottom

	- ball_y ball_vy
	neg ball_vy
	play_mml wall_sfx

label no_bounce_bottom

	var number paddle_top
	var number paddle_bottom
	var number paddle_x

	= paddle_top paddle1
	- paddle_top half
	- paddle_top ball_radius
	= paddle_bottom paddle_top
	+ paddle_bottom paddle_h
	+ paddle_bottom ball_radius
	+ paddle_bottom ball_radius

	? ball_y paddle_top
	jl done_hit_paddle1

	? ball_y paddle_bottom
	jg done_hit_paddle1

	= paddle_x paddle_w
	* paddle_x 2
	+ paddle_x ball_radius
	? ball_x paddle_x
	jg done_hit_paddle1

	? prev_ball_x paddle_x
	jl done_hit_paddle1

	neg ball_vx
	+ ball_vx 0.25
	+ ball_x ball_vx
	play_mml wall_sfx

label done_hit_paddle1

	= paddle_top paddle2
	- paddle_top half
	- paddle_top ball_radius
	= paddle_bottom paddle_top
	+ paddle_bottom paddle_h
	+ paddle_bottom ball_radius
	+ paddle_bottom ball_radius

	? ball_y paddle_top
	jl done_hit_paddle2

	? ball_y paddle_bottom
	jg done_hit_paddle2

	= paddle_x 640
	- paddle_x paddle_w
	- paddle_x paddle_w
	- paddle_x ball_radius
	? ball_x paddle_x
	jl done_hit_paddle2

	? prev_ball_x paddle_x
	jg done_hit_paddle2

	neg ball_vx
	- ball_vx 0.25
	+ ball_x ball_vx
	play_mml wall_sfx

label done_hit_paddle2

	= prev_ball_x ball_x
	= prev_ball_y ball_y
end

function draw
start
	clear 0 0 0

	var number y
	= y paddle1
	- y half

	filled_rectangle 128 128 128 255 128 128 128 255 128 128 128 255 128 128 128 255 paddle_w y paddle_w paddle_h

	var number x
	= x 640
	- x paddle_w
	- x paddle_w

	= y paddle2
	- y half

	filled_rectangle 128 128 128 255 128 128 128 255 128 128 128 255 128 128 128 255 x y paddle_w paddle_h

	var number bx
	var number by
	var number bw
	var number bh
	= bx ball_x
	- bx ball_radius
	= by ball_y
	- by ball_radius
	= bw ball_radius
	* bw 2

	filled_rectangle 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 bx by bw bw
end

call reset_ball
