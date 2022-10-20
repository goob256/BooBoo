robot = load_image("misc/robot.tga")
grass = load_image("misc/grass.tga")

w = 640 / 20
h = 360 / 20

function draw()
	image_start(grass)
	for y=0,h-1 do
		for x=0,w-1 do
			image_draw(grass, 255, 255, 255, 255, x*20, y*20, 0, 0)
		end
	end
	image_end(grass)

	for i=1,16 do
		x = rand(0, w-1)
		y = rand(0, h-1)
		image_draw(robot, 255, 255, 255, 255, x*20, y*20, 0, 0)
	end
end
