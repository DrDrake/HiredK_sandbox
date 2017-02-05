--[[
- @file planet_producer.h
- @brief
]]

class 'planet_producer' (root.TileProducer)

function planet_producer:__init(planet, terrain, seed)
    root.TileProducer.__init(self, terrain, 101)
	self.planet_ = planet
	
	self.perlin_noise_ = root.ImprovedPerlinNoise(256)
	self.perlin_noise_:generate(seed)
	
	self.noise_amp_ = { -3250.0, -1590.0, -1125.0, -795.0, -561.0, -397.0, -140.0, -100.0, 15.0, 8.0, 5.0, 2.5, 1.5, 1.0 }    
	self.noise_amp_factor_ = self.planet_.radius_ * 0.00000065
	self.noise_frequency_ = 40.0
	self.splat_upsample_factor_ = 8.0
	self.modified_residual_tex_ = {}
	self.fbo_ = root.FrameBuffer()
	
	self.road_graph_ = root.Graph()
	self.road_height_layer_ = road_height_layer(self, self.road_graph_)
	self.road_ortho_layer_ = road_ortho_layer(self, self.road_graph_)
	self.road_curve_start_ = vec3() -- xy: coordinates; z: indicate if new curve
	self.road_default_width_ = 7.0
	
	local road_graph_path = self:get_road_graph_path()
	if root.FileSystem.exists(road_graph_path) then
		self.road_graph_:read(FileSystem:search(road_graph_path, true))
	end
	
	self.height_shader_ = FileSystem:search("shaders/producer/height.glsl", true)
	self.normal_shader_ = FileSystem:search("shaders/producer/normal.glsl", true)
    self.splat_shader_ = FileSystem:search("shaders/producer/splat.glsl", true)
	self.brush_shader_ = FileSystem:search("shaders/producer/brush_ortho.glsl", true)
	self.screen_quad_ = root.Mesh.build_quad()
end

function planet_producer:get_residual_path_from_tile(tile)
	return string.format("%s/%d_%d%d%d.raw", root.FileSystem.remove_ext(self.planet_.path_), self.owner.face, tile.tx, tile.ty, tile.level)
end

function planet_producer:get_road_graph_path()
	return string.format("%s/%d_road.graph", root.FileSystem.remove_ext(self.planet_.path_), self.owner.face)
end

function planet_producer:__produce(tile)

	local residual_tex_path = self:get_residual_path_from_tile(tile)
	local residual_tex_settings = { iformat = gl.RG32F, format = gl.RED, type = gl.FLOAT, filter_mode = gl.LINEAR }
	local residual_tex = nil
	
	if root.FileSystem.exists(residual_tex_path) then
		residual_tex = root.Texture.from_raw(self.tile_size, self.tile_size, FileSystem:search(residual_tex_path, true), residual_tex_settings)
	else		
		residual_tex = root.Texture.from_empty(self.tile_size, self.tile_size, residual_tex_settings)
	end
	
	tile:set_texture(residual_tex, 3)	
	self:process_height_pass(tile)
	self:process_normal_pass(tile)
    self:process_splat_pass(tile)
	return true
end

function planet_producer:process_height_pass(tile)

	local parent_tile = nil
	if tile.level > 0 then
		parent_tile = self:get_tile(tile.level - 1, tile.tx / 2, tile.ty / 2)
		if parent_tile == nil then
			print("planet_producer:__produce(tile) - Out of sync!")
			return false
		end
	end

	local height_tex = root.Texture.from_empty(self.tile_size, self.tile_size, { iformat = gl.RG32F, format = gl.RG, type = gl.FLOAT, filter_mode = gl.LINEAR })
		
	self.fbo_:clear()
	self.fbo_:attach(height_tex, gl.COLOR_ATTACHMENT0)
	self.fbo_:bind_output()
	
	self.height_shader_:bind()
	
	local l = self.owner.root.coord.length
	local size = self.tile_size - (1 + self.border * 2)
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("_PermTable2D", 0)
	self.perlin_noise_:bind(root.ImprovedPerlinNoise.PERM_TABLE)
	
	gl.ActiveTexture(gl.TEXTURE1)
	root.Shader.get():sampler("_Gradient3D", 1)
	self.perlin_noise_:bind(root.ImprovedPerlinNoise.GRADIENT)
	
	if tile.level > 0 then
	
		gl.ActiveTexture(gl.TEXTURE2)
		root.Shader.get():sampler("_CoarseLevelSampler", 2)
		parent_tile:get_texture(0):bind()
	
		local dx = (tile.tx % 2) * (size / 2)
		local dy = (tile.ty % 2) * (size / 2)
		local coarseLevelOSL = vec4(dx / self.tile_size, dy / self.tile_size, 1.0 / self.tile_size, 0.0)
		root.Shader.get():uniform("_CoarseLevelOSL", coarseLevelOSL)
	else
		root.Shader.get():uniform("_CoarseLevelOSL", -1.0, -1.0, -1.0, -1.0)
	end
	
	gl.ActiveTexture(gl.TEXTURE3)
	root.Shader.get():sampler("_ResidualSampler", 3)
	tile:get_texture(3):bind()
	
	root.Shader.get():uniform("_ResidualOSH", 0.25 / self.tile_size, 0.25 / self.tile_size, 2.0 / self.tile_size, 1.0)
	
	local tile_wsd = vec4()
	tile_wsd.x = self.tile_size
	tile_wsd.y = l / bit.lshift(1, tile.level) / size
	tile_wsd.z = size / (25 - 1)
	tile_wsd.w = 0.0
	
	local offset = vec4()
	offset.x = (tile.tx / bit.lshift(1, tile.level) - 0.5) * l
	offset.y = (tile.ty / bit.lshift(1, tile.level) - 0.5) * l
	offset.z = l / bit.lshift(1, tile.level)
	offset.w = l / 2
	
	local rs = (tile.level < 14) and self.noise_amp_[tile.level + 1] or 0.0
	
	root.Shader.get():uniform("_LocalToWorld", mat4(self.owner:get_face_matrix()))	
	root.Shader.get():uniform("_TileWSD", tile_wsd)
	root.Shader.get():uniform("_Offset", offset)
	root.Shader.get():uniform("_Frequency", self.noise_frequency_ * bit.lshift(1, tile.level))
	root.Shader.get():uniform("_Amp", rs * self.noise_amp_factor_)
	
	self.screen_quad_:draw()
	root.Shader.pop_stack()
	
	if tile.level == 14 then
		self.road_height_layer_:__blit(tile.level, tile.tx, tile.ty, self.tile_size)
	end
	
	tile:set_texture(height_tex, 0)
end

function planet_producer:process_normal_pass(tile)

	local normal_tex = root.Texture.from_empty(self.tile_size, self.tile_size, { iformat = gl.RG8, format = gl.RG, filter_mode = gl.LINEAR })
		
	self.fbo_:clear()
	self.fbo_:attach(normal_tex, gl.COLOR_ATTACHMENT0)
	self.fbo_:bind_output()
	
	self.normal_shader_:bind()
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("_ElevationSampler", 0)
	tile:get_texture(0):bind()
	
	local D = self.owner.root.coord.length
	local R = D / 2.0

	local x0 = tile.tx / bit.lshift(1, tile.level) * D - R
	local x1 = (tile.tx + 1) / bit.lshift(1, tile.level) * D - R
	local y0 = tile.ty / bit.lshift(1, tile.level) * D - R
	local y1 = (tile.ty + 1) / bit.lshift(1, tile.level) * D - R
	
	local p0 = vec3(x0, y0, R)
	local p1 = vec3(x1, y0, R)
	local p2 = vec3(x0, y1, R)
	local p3 = vec3(x1, y1, R)
	local pc = vec3((x0 + x1) * 0.5, (y0 + y1) * 0.5, R)
	
	local v0 = math.normalize(p0)
	local v1 = math.normalize(p1)
	local v2 = math.normalize(p2)
	local v3 = math.normalize(p3)
	local vc = (v0 + v1 + v2 + v3) * 0.25
	
	local uz = math.normalize(pc)
	local ux = math.normalize(math.cross(vec3(0, 1, 0), uz))
	local uy = math.cross(uz, ux)
	
	local deformedCorners = math.transpose(mat4.from_table({
		v0.x * R - vc.x * R, v1.x * R - vc.x * R, v2.x * R - vc.x * R, v3.x * R - vc.x * R,
		v0.y * R - vc.y * R, v1.y * R - vc.y * R, v2.y * R - vc.y * R, v3.y * R - vc.y * R,
		v0.z * R - vc.z * R, v1.z * R - vc.z * R, v2.z * R - vc.z * R, v3.z * R - vc.z * R,
		1.0, 1.0, 1.0, 1.0
	}))
	
	local deformedVerticals = math.transpose(mat4.from_table({
		v0.x, v1.x, v2.x, v3.x,
		v0.y, v1.y, v2.y, v3.y,
		v0.z, v1.z, v2.z, v3.z,
		0.0, 0.0, 0.0, 0.0
	}))
	
	local worldToTangentFrame = math.transpose(mat4.from_table({
		ux.x, ux.y, ux.z, 0.0,
		uy.x, uy.y, uy.z, 0.0,
		uz.x, uz.y, uz.z, 0.0,
		0.0, 0.0, 0.0, 0.0
	}))
	
	root.Shader.get():uniform("_PatchCorners", deformedCorners)
	root.Shader.get():uniform("_PatchVerticals", deformedVerticals)	
	root.Shader.get():uniform("_PatchCornerNorms", math.length(p0), math.length(p1), math.length(p2), math.length(p3))
	root.Shader.get():uniform("_WorldToTangentFrame", worldToTangentFrame)
	
	root.Shader.get():uniform("_TileSD", self.tile_size, (self.tile_size - 1) / (25 - 1))
	root.Shader.get():uniform("_ElevationOSL", 0.25 / self.tile_size, 0.25 / self.tile_size, 1.0 / self.tile_size, 0.0)
	root.Shader.get():uniform("_Deform", x0, y0, D / bit.lshift(1, tile.level), R)
	
	self.screen_quad_:draw()
	root.Shader.pop_stack()		
	
	tile:set_texture(normal_tex, 1)	
end

function planet_producer:process_splat_pass(tile)

	local upsample_size = self.tile_size * self.splat_upsample_factor_
	local splat_tex = root.Texture.from_empty(upsample_size, upsample_size, { filter_mode = gl.LINEAR })
	
	self.fbo_:clear()
	self.fbo_:attach(splat_tex, gl.COLOR_ATTACHMENT0)
	self.fbo_:bind_output()
	
	gl.PushAttrib(gl.VIEWPORT_BIT)
	gl.Viewport(0, 0, upsample_size, upsample_size)      
	self.splat_shader_:bind()
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("_ElevationSampler", 0)
	tile:get_texture(0):bind()
	
	gl.ActiveTexture(gl.TEXTURE1)
	root.Shader.get():sampler("_NormalSampler", 1)
	tile:get_texture(1):bind()
	
	self.screen_quad_:draw()
	root.Shader.pop_stack()
	
	if tile.level == 14 then
		self.road_ortho_layer_:__blit(tile.level, tile.tx, tile.ty, upsample_size)
	end
	
	tile:set_texture(splat_tex, 2)	
	gl.PopAttrib()
end

function planet_producer:edit_residual(tile, stroke, scale)

	local bounds = Box2D(vec2(tile.coord.ox, tile.coord.oy), vec2(tile.coord.ox, tile.coord.oy) + vec2(tile.coord.length))	
	local stroke_bounds = Box2D(vec2(stroke.x, stroke.z) - (stroke.w * 0.5), vec2(stroke.x, stroke.z) + (stroke.w * 0.5))
	if not bounds:intersects(stroke_bounds) then
		return
	end
	
	if tile:is_leaf() then
		
		gl.PushAttrib(gl.VIEWPORT_BIT)
		gl.Viewport(0, 0, self.tile_size, self.tile_size)
		self.brush_shader_:bind()
		
		gl.BlendFunc(gl.ONE, gl.ONE)
        gl.Enable(gl.BLEND)
		
		local a = self.tile_size / (self.tile_size - 5.0)
		local b = -2.5 / (self.tile_size - 5.0)
		
		root.Shader.get():uniform("u_Offset", a / 2.0, a / 2.0, b + a / 2.0, b + a / 2.0)
		root.Shader.get():uniform("u_DeformOffset", tile.coord.ox, tile.coord.oy, tile.coord.length, tile.level)
		root.Shader.get():uniform("u_Stroke", stroke)
		root.Shader.get():uniform("u_StrokeEnd", stroke)
		root.Shader.get():uniform("u_Scale", scale)

		local data = self:get_tile(tile.level, tile.coord.tx, tile.coord.ty)
		local residual_tex = data:get_texture(3)
		
		self.fbo_:clear()
		self.fbo_:attach(residual_tex, gl.COLOR_ATTACHMENT0)
		self.fbo_:bind_output()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
		
        gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
		gl.Disable(gl.BLEND)
		
		-- mark residual tex as modified so that it can be saved
		local hash = root.TileProducer.TileData.get_hash(tile.level, tile.coord.tx, tile.coord.ty)
		self.modified_residual_tex_[hash] = residual_tex
		self.modified_residual_tex_[hash].tile_ = data
		
		-- refresh tile textures
		self:process_height_pass(data)
		self:process_normal_pass(data)
		self:process_splat_pass(data)		
		root.FrameBuffer.unbind()
		gl.PopAttrib()
		return
	end
	
	for i = 1, 4 do
		self:edit_residual(tile:get_child(i - 1), stroke, scale)
	end
end

function planet_producer:edit_road_curve(tile, coord)

	local function refresh_affected_tiles(tile, curve)
	
		local bounds = Box2D(vec2(tile.coord.ox, tile.coord.oy), vec2(tile.coord.ox, tile.coord.oy) + vec2(tile.coord.length))
		if not bounds:intersects(curve.bounds) then
			return
		end
		
		if tile:is_leaf() then
		
			local data = self:get_tile(tile.level, tile.coord.tx, tile.coord.ty)
			
			gl.PushAttrib(gl.VIEWPORT_BIT)
			gl.Viewport(0, 0, self.tile_size, self.tile_size)
			
			-- refresh tile textures
			self:process_height_pass(data)
			self:process_normal_pass(data)
			self:process_splat_pass(data)		
			root.FrameBuffer.unbind()
			gl.PopAttrib()
			return
		end
		
		for i = 1, 4 do
			refresh_affected_tiles(tile:get_child(i - 1), curve)
		end
	end

	if self.road_curve_start_.z == 0.0 then		
		self.road_curve_start_ = vec3(coord.x, coord.z, 1.0)
	else	
		local curve = self.road_graph_:add_curve(vec2(self.road_curve_start_) * 0.5, vec2(coord.x, coord.z) * 0.5) -- temp
		curve.width = self.road_default_width_
		self.road_curve_start_.z = 0.0
		
		refresh_affected_tiles(tile, curve)
	end
end

function planet_producer:write_modified_data()

	for k, v in pairs(self.modified_residual_tex_) do
		self:write_residual_texture(self:get_residual_path_from_tile(v.tile_), v)
	end
	
	self.road_graph_:write(self:get_road_graph_path())
	self.modified_residual_tex_ = {}
end