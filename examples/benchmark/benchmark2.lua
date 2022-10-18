function draw_at(r, g, b, x, radius)
	f = x / 640 * 3.14159 * 2
	f = math.sin(f)
	f = f * 90 + 180
	filled_circle(r, g, b, 255, x, f, radius, -1)
end

function draw()
	clear(0, 0, 0)

	xx = 0

	start_primitives()

	for xx=0,640,2 do
		draw_at(0, 255, 0, xx, 8)
	end

	tmp = x % 640
	draw_at(128, 255, 128, tmp, 32)

	end_primitives()
end

function run()
	x = x + 5
end

x = 0
