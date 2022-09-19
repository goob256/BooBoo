+ __count__ 1
? __count__ __slideshow_delay__
jl __no_slide__
? __do_slideshow__ 0
je __no_slide__
reset reset_game_name
:__no_slide__
var number __joy_x1
var number __joy_y1
var number __joy_x2
var number __joy_y2
var number __joy_x3
var number __joy_y3
var number __joy_l
var number __joy_r
var number __joy_u
var number __joy_d
var number __joy_a
var number __joy_b
var number __joy_x
var number __joy_y
var number __joy_lb
var number __joy_rb
var number __joy_ls
var number __joy_rs
var number __joy_back
var number __joy_start
joystick_poll 0 __joy_x1 __joy_y1 __joy_x2 __joy_y2 __joy_x3 __joy_y3 __joy_l __joy_r __joy_u __joy_d __joy_a __joy_b __joy_x __joy_y __joy_lb __joy_rb __joy_ls __joy_rs __joy_back __joy_start
? __joy_back 0
je __no_main__
reset "main.boo"
:__no_main__
