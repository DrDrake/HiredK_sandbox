--[[
- @file road_graph_layers.h
- @brief
]]

--[[
////////////////////////////////////////////////////////////////////////////////
// road_height_layer implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'road_height_layer' (root.GraphLayer)

function road_height_layer:__init(producer, graph)
    root.GraphLayer.__init(self, producer)
	self.graph_ = graph
	
	self.shader_ = FileSystem:search("shaders/producer/road_height.glsl", true)
end

function road_height_layer:__blit(level, tx, ty, size)

	local q = self:get_tile_coords(level, tx, ty)	
	local nx = vec2()
	local ny = vec2()
	local lx = vec2()
	local ly = vec2()
	--self:get_deformed_parameters(q, nx, ny, lx, ly)
	
	local scale = 2.0 * (size - 1.0 - (2.0 * self.producer.border)) / q.z
	local offset = vec3(q.x + q.z / 2.0, q.y + q.z / 2.0, scale / size)
	
	gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
	gl.Enable(gl.BLEND)
	self.shader_:bind()
	
	local ci = self.graph_.curves
	while ci:has_next() do
	
		local p = ci:next()		
		local w = p.width + 2.0 * math.sqrt(2.0) / scale
		local tw = w * 3.0
		
		self:draw_curve_altitude(offset, p, tw, tw / w, math.max(1.0, 1.0 / scale), true, nx, ny, lx, ly)
	end
	
	root.Shader.pop_stack()
	gl.Disable(gl.BLEND)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// road_ortho_layer implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'road_ortho_layer' (root.GraphLayer)

function road_ortho_layer:__init(producer, graph)
    root.GraphLayer.__init(self, producer)
	self.graph_ = graph
	
	self.shader_ = FileSystem:search("shaders/producer/road_ortho.glsl", true)
	self.border_width_ = vec2(1.2, 2.0) -- width / inner width
end

function road_ortho_layer:__blit(level, tx, ty, size)

	local q = self:get_tile_coords(level, tx, ty)
	local nx = vec2()
	local ny = vec2()
	local lx = vec2()
	local ly = vec2()
	--self:get_deformed_parameters(q, nx, ny, lx, ly)
	
	local scale = 2.0 * (1.0 - (self.producer.border * 8) * 2.0 / size) / q.z -- todo: upsample border
	local offset = vec3(q.x + q.z / 2.0, q.y + q.z / 2.0, scale)	
	local scale2 = scale * size
	
	local function draw_roads_border()
	
		gl.BlendFunc(gl.ONE, gl.ONE)
		gl.Enable(gl.BLEND)
		
		for pass = 1, 2 do
		
			local ci = self.graph_.curves
			while ci:has_next() do
			
				local p = ci:next()
				local swidth = p.width * scale2	
				
				if swidth > 2.0 and p.width > 1.0 then
				
                    local b0 = p.width * self.border_width_.x / 2
					local b1 = p.width * self.border_width_.y / 2
                    local a = 1.0 / (b1 - b0)
                    local b = -b0 * a
					
					local blend_size = (pass == 1) and vec2(-a, 1.0 - b) or vec2()
					root.Shader.get():uniform("u_BlendSize", blend_size)
					
					local w = p.width * self.border_width_.x				
					self:draw_curve(offset, p, w, p.width, scale2, nx, ny, lx, ly)
				end
			end
		end
		
		root.Shader.get():uniform("u_BlendSize", vec2())
		gl.Disable(gl.BLEND)
	end
	
	local function draw_roads()
	
		local ci = self.graph_.curves
		while ci:has_next() do
		
			local p = ci:next()
			self:draw_curve(offset, p, p.width, 0, scale2, nx, ny, lx, ly)
		end
	end
	
	self.shader_:bind()
	draw_roads_border()
	draw_roads()	
	root.Shader.pop_stack()
end