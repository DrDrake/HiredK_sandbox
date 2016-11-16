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
	self.noise_amp_factor_ = self.planet_.radius_ * 0.0000004
	self.noise_frequency_ = 40.0
	self.fbo_ = root.FrameBuffer()
	
	self.height_shader_ = FileSystem:search("shaders/height.glsl", true)
	self.normal_shader_ = FileSystem:search("shaders/normal.glsl", true)
	self.screen_quad_ = root.Mesh.build_quad()
end

function planet_producer:__produce(tile)

	local parent_tile = nil
	if tile.level > 0 then
		parent_tile = self:get_tile(tile.level - 1, tile.tx / 2, tile.ty / 2)
		if parent_tile == nil then
			print("planet_producer:__produce(tile) - Out of sync!")
			return false
		end
	end
	
	local process_height_pass = function()
	
		local height_tex = root.Texture.from_empty(self.tile_size, self.tile_size,
				{ iformat = gl.RG16F, format = gl.RG, type = gl.HALF_FLOAT, filter_mode = gl.LINEAR })
				
		self.fbo_:clear()
		self.fbo_:attach(height_tex, gl.COLOR_ATTACHMENT0)
		self.fbo_:bind_output()
		
		local l = self.owner.root.coord.length
		local size = self.tile_size - (1 + self.border * 2)
		self.height_shader_:bind()
		
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
		tile:add_texture(height_tex)
		root.Shader.pop_stack()
	end
	
	local process_normal_pass = function()
	
		local normal_tex = root.Texture.from_empty(self.tile_size, self.tile_size,
			{ iformat = gl.RG8, format = gl.RG, filter_mode = gl.LINEAR })
			
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
		tile:add_texture(normal_tex)	
		root.Shader.pop_stack()		
	end
	
	process_height_pass()
	process_normal_pass()
	return true
end