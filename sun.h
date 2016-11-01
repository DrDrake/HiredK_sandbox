--[[
- @file sun.h
- @brief
]]

class 'sun' (root.ScriptObject)

function sun:__init(elevation, orientation, resolution, num_splits)
    root.ScriptObject.__init(self, "sun")
	self.type = root.ScriptObject.Dynamic
	
	self.coord_ = vec2(elevation, orientation)
	self.shadow_splits_ = { 1.0, 20.0, 100.0, 500.0, 1500.0 }
	self.resolution_ = resolution
	self.num_splits_ = num_splits
	
	self.shadow_depth_tex_array_ = root.Texture.from_empty(resolution, resolution, num_splits, { target = gl.TEXTURE_2D_ARRAY, iformat = gl.DEPTH_COMPONENT24, format = gl.DEPTH_COMPONENT, compare_mode = gl.LEQUAL })
	self.shadow_depth_fbo_ = root.FrameBuffer()
	self.shadow_splits_clip_space_ = {}
	self.shadow_matrices_ = {}
	
	self.shadow_bias_matrix_ = mat4.from_table({
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	})
end

function sun:get_direction()
	return vec3(0, 0, 1) * mat3(math.rotate(math.rotate(mat4(), math.radians(self.coord_.x + 90), vec3(1, 0, 0)), math.radians(self.coord_.y), vec3(0, 1, 0)))
end

function sun:__update(dt)

	if sf.Keyboard.is_key_pressed(sf.Keyboard.Up) then
		self.coord_.x = self.coord_.x - 15.0 * dt
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.Down) then
		self.coord_.x = self.coord_.x + 15.0 * dt
	end
	
	if sf.Keyboard.is_key_pressed(sf.Keyboard.Left) then
		self.coord_.y = self.coord_.y - 15.0 * dt
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.Right) then
		self.coord_.y = self.coord_.y + 15.0 * dt
	end
end

function sun:make_sun_shadow(camera, draw_shadow_func)

	gl.PushAttrib(gl.VIEWPORT_BIT)
	gl.Viewport(0, 0, self.resolution_, self.resolution_)
	gl.Enable(gl.POLYGON_OFFSET_FILL)
	
	self.shadow_splits_clip_space_ = {}
	self.shadow_matrices_ = {}

	for i = 1, self.num_splits_ do
	
		self.shadow_depth_fbo_:clear()
		self.shadow_depth_fbo_:attach(self.shadow_depth_tex_array_, gl.DEPTH_ATTACHMENT, 0, i - 1)
		self.shadow_depth_fbo_:bind_output()
		gl.Clear(gl.DEPTH_BUFFER_BIT)
		gl.PolygonOffset(i, 4096.0)
		
		local size = self.shadow_splits_[i + 1]
		local clip = camera.proj_matrix * vec4(0, 0, -size, 1)
		self.shadow_splits_clip_space_[i] = (clip / clip.w).z
		
		local sun_view = math.lookat((camera.position - math.normalize(self:get_direction())), camera.position, vec3(0, 1, 0))
		local sun_proj = math.ortho(-size, size, -size, size, 1000, -size)
		local origin = sun_proj * sun_view * vec4(0, 0, 0, 1)
		origin = origin * (self.resolution_ / 2.0)
		
		local rounded_origin = math.round(origin)
		local round_offset = (rounded_origin - origin) * (2.0 / self.resolution_)
		round_offset.z = 0.0
		round_offset.w = 0.0
		
		sun_proj:set(3, sun_proj:get(3) + round_offset)
		self.shadow_matrices_[i] = self.shadow_bias_matrix_ * sun_proj * sun_view
		draw_shadow_func(sun_proj * sun_view)
	end
	
	gl.Disable(gl.POLYGON_OFFSET_FILL)
	root.FrameBuffer.unbind()
	gl.PopAttrib()
end