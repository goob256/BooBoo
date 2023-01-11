BooBoo is a mini high-level assembly-like language with a game/multimedia
API.

To run the benchmarks, you should add -vsync +fps to the command line.

To download a binary with examples, go to https://b1stable.com.

If you wish to distribute the binary (or your own compiled binaries) as
a standalone app (with your main.boo,) you are allowed to do that.

------------------------------------------------------------------------------

There are 3 functions taking/returning no parameters called from the app:

draw - called at refresh rate
run - called 60 times per second
end - called when the program is exiting

At startup, global code is run. Then the above mentioned callbacks get called
at intervals or shutdown.

The screen is 640x360.

There is a little bit of joystick emulation on the keyboard:
	WASD/space is X1/Y1/A
	LRUD/return are X2/Y2/B
	LRUD also send DPAD buttons
	Escape is Back

Some of the demo games will auto-play by an AI if a joystick isn't connected.
I'll leave it as an exercise if you want to play them with keyboard.

The following sections describe the API:
	Core Functions
	Math Functions
	Graphics Functions
	Primitives Functions
	Image Functions
	Font Functions
	Audio Functions
	Joystick Functions
	Vector Functions
	Configuration Functions

-- Core Functions ------------------------------------------------------------

reset
	load a different BooBoo script
	e.g. reset "next_slide.boo"

exit
	Exit the app with a return code to the environment
	e.g. exit 0

return
	Return a value from a function
	e.g. return 1

number
	define a number
	e.g. number x

string
	define a string
	e.g. string s

vector
	define a vector
	e.g. vector x

=
	assign
	e.g. = x 1

+
	add
	e.g. + x 1

-
	subtract
	e.g. - x 1

*
	multiply
	e.g. * x 2

/
	divide
	e.g. / x 2

%
	integer modulus
	e.g. % index count

fmod
	floating point modulus
	e.g. % f 2.5

neg
	negate
	e.g. neg x

:
	define a label
	e.g. :loop

goto
	jump to a label

?
	comparison
	e.g. ? x 0

je
	check comparison flag, jump if equal
	e.g. je label

jne
	check comparison flag, jump if not equal
	e.g. jne label

jle
	check comparison flag, jump if less than or equal
	e.g. jle label

jge
	check comparison flag, jump if greator or equal
	e.g. jge label

jl
	check comparison flag, jump if less
	e.g. jl label

jg
	check comparison flag, jump if greater
	e.g. jg label

function
	define a function. after function comes the function name, followed by variables, then {, then the function body, and finally }
	e.g.
		function twice x
		{
			* x 2
			return x
		}

call
	call a function, taking no return
	e.g. call my_func param1 param2

call_result
	call a function that takes a return value
	e.g. call_result my_result my_func param1 param2 param3

;
	comment
	e.g. = x 1 ; just a comment

include
	insert a file of BooBoo code here
	e.g. include "my_helpful_things.inc"

inspect
	print a value in a popup for debugging
	e.g. inspect x

string_format
	format strings with replacements
	e.g. string_format dest "I'm % years old" age

delay
	delay an amount of milliseconds
	e.g. delay 1000

-- Math Functions ------------------------------------------------------------

sin
	trigonometry functions
	e.g. sin dest 3.14159

cos
	trigonometry functions
	e.g. cos dest 3.14159

atan2
	trigonometry functions
	e.g. atan2 y x

abs
	absolute value
	e.g. abs dest value

pow
	exponentiation
	e.g. pow x 2

sqrt
	square root
	e.g. sqrt x 256

rand
	random integers between two numbers inclusive
	e.g. rand r 0 99

-- Graphics Functions --------------------------------------------------------

clear
	clear the screen
	e.g. clear r g b

flip
	flip the backbuffer to the screen
	e.g. flip

-- Primitive Functions -------------------------------------------------------

start_primitives
	batch primitives for speed
	e.g. start_primitives

end_primitives
	batch primitives for speed
	e.g. end_primitives

line
	draw a line
	e.g. line r g b a x1 y1 x2 y2 thickness

filled_triangle
	draw a filled triangle with 3 colours
	e.g. filled_triangle r1 g1 b1 a1 r2 g2 b2 a2 r3 g3 b3 a3 x1 y1 x2 y2 x3 y3

rectangle
	draw a outline rectangle
	e.g. rectangle r g b a x y w h thickness

filled_rectangle
	draw a filled rectangle
	e.g. filled_rectangle r1 g1 b1 a1 r2 g2 b2 a2 r2 g3 b3 a3 r4 g4 b4 a4 x y w h

ellipse
	draw a outline ellipse with give number of pie slices (-1 for auto)
	e.g. ellipse r g b a x y radius_x radius_y thickness sections

filled_ellipse
	draw a filled ellipse with given number of pie slices (-1 for auto)
	e.g. filled_ellipse r g b a x y radius_x radius_y sections

circle
	draw a outline circle with given number of pie slices (-1 for auto)
	e.g. circle r g b a x y radius thickness sections

filled_circle
	draw a filled circle with give number of pie slices (-1 for auto)
	e.g. filled_circle r g b a x y radius sections

-- Image Functions -----------------------------------------------------------

image_load
	load an image
	e.g. image_load dest filename

image_draw
	draw an image
	e.g. image_draw img tint_r tint_g tint_b tint_a x y flip_h flip_v

image_stretch_region
	draw a region of an image and maybe stretch it
	e.g. image_stretch_region image r g b a source_x source_y source_w source_h dest_x dest_y dest_w dest_h flip_h flip_v

image_draw_rotated_scaled
	draw an image with rotation and scale
	e.g. image_draw_rotated_scaled img tint_r tint_g tint_b tint_a pivot_x pivot_y x y angle scale_x scale_y flip_h flip_v

image_start
	start a batch of images for speed (must be same image)

image_end
	end a batch of images

image_size
	get image width/height
	e.g. image_size img w h

-- Font Functions ------------------------------------------------------------

font_load
	load a TTF font
	e.g. font_load font "DejaVuSans.ttf" px_size smooth_flag

font_draw
	draw text
	e.g.
	font_draw font r g b a "HELLO" x y

font_width
	get width of a string given a font
	e.g. font_width font dest "FOOBARBAZ"

font_height
	get the height of a font
	e.g. font_height font dest

-- Audio Functions -----------------------------------------------------------

mml_create
	create an MML from a string
	e.g. mml_create dest "A @TYPE1 abcdefg"

mml_load
	load an MML from file
	e.g. mml_load dest "sfx/blast.mml"

mml_play
	play a sound
	e.g. mml_play blast volume loop

mml_stop
	stop a looping sound
	e.g. mml_stop blast

-- Joystick Functions --------------------------------------------------------

joystick_poll
	poll a joystick for input
	e.g. joystick_poll joystick_number x1 y1 x2 y2 x3 y3 l r u d a b x y lb rb ls rs back start

joystick_count
	get number of connected joysticks
	e.g. joystick_count num_joysticks

-- Vector Functions ----------------------------------------------------------

vector_add
	push a value onto a vector (must be created e.g. var vector v)
	e.g. vector_add v 0

vector_size
	get the number of elements in a vector
	e.g. vector_size vec size

vector_set
	set an element in a vector
	e.g. vector_set vec index value

vector_insert
	insert a value in any position in a vector
	e.g. vector_insert v index value

vector_get
	get the value at index in vector
	e.g. vector_get v index

vector_erase
	remove from a vector
	e.g. vector_erase v index

vector_clear
	empty a vector
	e.g. vector_clear v

-- Configuration Functions ---------------------------------------------------

cfg_load
	load a config file checking for success
	e.g. cfg_load success "com.b1stable.example"

cfg_save
	save a config file
	e.g. cfg_save "com.b1stable.example"

cfg_get_number
	read a number from config
	e.g. cfg_get_number dest "x"

cfg_get_string
	read a string from config
	e.g. cfg_get_string dest "name"

cfg_set_number
	set a value to be saved in a config file
	e.g. cfg_set_number "x" 1

cfg_set_string
	set a string value in a config file
	e.g. cfg_set_string "name" "Trent"

cfg_number_exists
	check if a number value exists in a config file
	e.g. cfg_number_exists "x"
	
cfg_string_exists
	check if a string name exists in config
	e.g. cfg_string_exists "name"
