number __do_slideshow__
= __do_slideshow__ 1
number __slideshow_delay__
= __slideshow_delay__ 600
number __count__
= __count__ 0
number __success__
cfg_load __success__ "com.b1stable.slideshow"
? __success__ 0
je __create__
cfg_get_number __do_slideshow__ "enabled"
cfg_get_number __slideshow_delay__ "delay"
goto __done__
:__create__
cfg_set_number "enabled" __do_slideshow__
cfg_set_number "delay" __slideshow_delay__
cfg_save "com.b1stable.slideshow"
:__done__
