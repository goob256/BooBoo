var number __do_slideshow__
= __do_slideshow__ 1
var number __count__
= __count__ 0
var number __success__
cfg_load __success__ "com.b1stable.slideshow"
? __success__ 0
je __create__
cfg_get_number __do_slideshow__ "do_slideshow"
goto __done__
label __create__
cfg_set_number "do_slideshow" 1
cfg_save "com.b1stable.slideshow"
label __done__
