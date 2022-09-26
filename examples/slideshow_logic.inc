+ __count__ 1
? __count__ __slideshow_delay__
jl __no_slide__
? __do_slideshow__ 0
je __no_slide__
reset reset_game_name
:__no_slide__
include "poll_joystick.inc"
? joy_back 0
je __no_main__
reset "main.boo"
:__no_main__
