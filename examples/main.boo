var vector programs
vector_add programs "blip.boo"
vector_add programs "clock.boo"
vector_add programs "gameover.boo"
vector_add programs "pong.boo"
vector_add programs "robots.boo"
vector_add programs "walk.boo"
vector_add programs "secret.boo"
vector_add programs "sine.boo"
vector_add programs "sneaky.boo"
var number size
vector_size programs size
var number index
- size 1
rand index 0 size
var string program
vector_get programs program index
reset program
