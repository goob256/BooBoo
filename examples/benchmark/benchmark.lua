grass = load_image("misc/grass.tga")
robot = load_image("misc/robot.tga")
img_w, img_h = image_size(grass)
w = 640 / img_w
h = 360 / img_h

function draw()
	image_start(grass)
	for y=0,h do
		for x=0,w do
			image_draw(grass, 255, 255, 255, 255, x*img_w, y*img_h, 0, 0);
		end
	end
	image_end(grass)

	for i=0,17 do
		x = rand(0, w-1)
		y = rand(0, h-1)
		image_draw(robot, 255, 255, 255, 255, x*img_w, y*img_h, 0, 0)
	end
end
