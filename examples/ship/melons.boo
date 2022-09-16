var string reset_game_name
= reset_game_name "blip.boo"
include "slideshow_start.inc"

var number was_pressed
= was_pressed 0

var number r
= r 0
var number g
= g 255
var number rinc
= rinc 16
var number ginc
= ginc -16

var number success
cfg_load success "com.b1stable.melons"
? success 0
je create
cfg_get_number r "r"
cfg_get_number g "g"
cfg_get_number rinc "rinc"
cfg_get_number ginc "ginc"
goto done_cfg
label create
cfg_set_number "r" r
cfg_set_number "g" g
cfg_set_number "rinc" rinc
cfg_set_number "ginc" ginc
label done_cfg

function logic
start
	include "poll_joystick.inc"

	? joy_a 1
	je currently_pressed

	= was_pressed 0
	goto finish

label currently_pressed
	? was_pressed 0
	je go
	goto finish

label go
	= was_pressed 1

	+ r rinc
	? r 256
	jl nope1
	neg rinc
	= r 255
	goto nope2
label nope1
	? r 0
	jge nope2
	= r 0
	neg rinc
label nope2

	+ g ginc
	? g 256
	jl nope3
	= g 255
	neg ginc
	goto nope4
label nope3
	? g 0
	jge nope4
	neg ginc
	= g 0
label nope4

label finish
	include "slideshow_logic.inc"
end

function draw
start
	clear r g 128
end

function shutdown
start
	cfg_set_number "r" r
	cfg_set_number "g" g
	cfg_set_number "rinc" rinc
	cfg_set_number "ginc" ginc
	cfg_save "com.b1stable.melons"
end
