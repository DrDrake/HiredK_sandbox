--[[
- @file object_lights.h
- @brief
]]

--[[
////////////////////////////////////////////////////////////////////////////////
// directional_light implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'directional_light' (object_base)

function directional_light:__init(planet, num_splits, resolution)
    object_base.__init(self, planet)
	self.type_id_ = ID_DIRECTIONAL_LIGHT
	self.is_light_ = true
    
    self.shader_ = FileSystem:search("shaders/pbr_directional.glsl", true)
	self.shadow_shader_ = FileSystem:search("shaders/gbuffer_simple.glsl", true)
	self.volume_ = root.Mesh.build_quad()
	self.num_splits_ = num_splits
	self.resolution_ = resolution
	self.coord_ = vec2(60.0, 20.0)

    self.shadow_fbo_ = root.FrameBuffer()
	self.shadow_tex_array_ = root.Texture.from_empty(resolution, resolution, num_splits, { target = gl.TEXTURE_2D_ARRAY, iformat = gl.DEPTH_COMPONENT24, format = gl.DEPTH_COMPONENT })
	self.shadow_splits_ = { 1.0, 40.0, 200.0, 500.0, 1500.0 }
	self.shadow_splits_clip_space_ = {}
	self.shadow_matrices_ = {}
	
	self.shadow_bias_matrix_ = mat4.from_table({
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	})
end

function directional_light:get_direction()
	return vec3(0, 0, 1) * mat3(math.rotate(math.rotate(mat4(), math.radians(self.coord_.x + 90), vec3(1, 0, 0)), math.radians(self.coord_.y), vec3(0, 1, 0)))
end

function directional_light:generate_shadow(camera)

	gl.PushAttrib(gl.VIEWPORT_BIT)
	gl.Viewport(0, 0, self.resolution_, self.resolution_)
	self.shadow_shader_:bind()
    
	gl.Enable(gl.POLYGON_OFFSET_FILL)
    gl.Enable(gl.DEPTH_TEST)
    gl.Enable(gl.CULL_FACE)
    gl.CullFace(gl.FRONT)
	
	self.shadow_splits_clip_space_ = {}
	self.shadow_matrices_ = {}
    
    for i = 1, self.num_splits_ do
    
		self.shadow_fbo_:clear()
		self.shadow_fbo_:attach(self.shadow_tex_array_, gl.DEPTH_ATTACHMENT, 0, i - 1)
		self.shadow_fbo_:bind_output()       
		gl.Clear(gl.DEPTH_BUFFER_BIT)
		gl.PolygonOffset(i, 4096.0)
        
		local size = self.shadow_splits_[i + 1]
		local clip = camera.proj_matrix * vec4(0, 0, -size, 1)
		self.shadow_splits_clip_space_[i] = (clip / clip.w).z
		
		local sun_view = math.lookat((camera.position - math.normalize(self:get_direction())), camera.position, vec3(0, 1, 0))
		local sun_proj = math.ortho(-size, size, -size, size, 500, -size)
		local origin = sun_proj * sun_view * vec4(0, 0, 0, 1)
		origin = origin * (self.resolution_ / 2.0)
		
		local rounded_origin = math.round(origin)
		local round_offset = (rounded_origin - origin) * (2.0 / self.resolution_)
		round_offset.z = 0.0
		round_offset.w = 0.0
		
		sun_proj:set(3, sun_proj:get(3) + round_offset)
		local shadow_matrix = sun_proj * sun_view	
		self.shadow_matrices_[i] = self.shadow_bias_matrix_ * shadow_matrix
		
		for i = 1, table.getn(self.planet_.active_sectors_) do
			local sector = self.planet_.active_sectors_[i]
			for j = 1, table.getn(sector.objects_) do
				sector.objects_[j]:draw_shadow(shadow_matrix)
			end
		end
		
		for i = 1, table.getn(self.planet_.objects_) do
			self.planet_.objects_[i]:draw_shadow(shadow_matrix)
		end
    end
    
	gl.Disable(gl.POLYGON_OFFSET_FILL)
    gl.Disable(gl.DEPTH_TEST)
    gl.Disable(gl.CULL_FACE)
    gl.CullFace(gl.BACK)	
	
	root.FrameBuffer.unbind()
	root.Shader.pop_stack()
	gl.PopAttrib()
end

function directional_light:draw_light(camera)

    root.Shader.get():uniform_mat4_array("u_LightShadowMatrix", self.shadow_matrices_)	
    root.Shader.get():uniform("u_LightShadowSplits", vec4.from_table(self.shadow_splits_clip_space_))
    root.Shader.get():uniform("u_LightShadowResolution", self.resolution_)
    root.Shader.get():uniform("u_LightDirection", self:get_direction())
	root.Shader.get():uniform("u_DeferredDist", (self.planet_.enable_logz_) and 0.0 or self.planet_.deferred_dist_clip_space_)
	self.planet_.atmosphere_:bind(1000)
	self.planet_.clouds_:bind_coverage()
	
	gl.ActiveTexture(gl.TEXTURE4)
    root.Shader.get():sampler("s_LightDepthTex", 4)
    self.shadow_tex_array_:bind()
	
    self.volume_:draw()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// point_light implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'point_light' (object_base)

function point_light:__init(planet)
    object_base.__init(self, planet)
	self.type_id_ = ID_POINT_LIGHT
	self.is_light_ = true
	
	self.shader_ = FileSystem:search("shaders/pbr_pointlight.glsl", true)
	self.radius_ = 15
	self.intensity_ = 50
	self.color_ = root.Color.White
	self:rebuild_volume()
end

function point_light:rebuild_volume()
	self.volume_ = root.Mesh.build_cube(self.radius_)
	self.transform_.box:set(vec3(self.radius_))
end

function point_light:set_radius(value)
	self.radius_ = value
	self:rebuild_volume()
end

function point_light:parse(object)

    self.radius_ = object:get_number("radius")
	self.intensity_ = object:get_number("intensity")
	self:rebuild_volume()
	
	object_base.parse(self, object)
end

function point_light:write(object)

	object:set_number("radius", self.radius_)
	object:set_number("intensity", self.intensity_)
	object_base.write(self, object)
end

function point_light:draw_light(camera)

    root.Shader.get():uniform("u_InvLightRadius", 1.0 / self.radius_)
    root.Shader.get():uniform("u_LightIntensity", self.intensity_)
	root.Shader.get():uniform("u_LightWorldPos", self.transform_.position)
	root.Shader.get():uniform("u_LightColor", self.color_)
	gl.CullFace(gl.FRONT)
	
    self.volume_:copy(self.transform_)
    self.volume_:draw(camera)
	gl.CullFace(gl.BACK)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// envprobe implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'envprobe' (object_base)

function envprobe:__init(planet)
    object_base.__init(self, planet)
	self.type_id_ = ID_ENVPROBE
	self.is_local_ = self.planet_.global_envprobe_ ~= nil
	self.is_light_ = true
	
	self.shader_ = FileSystem:search("shaders/pbr_envprobe.glsl", true)
	self.sky_shader_ = FileSystem:search("shaders/sky.glsl", true)
	self.gbuffer_shader_ = FileSystem:search("shaders/gbuffer.glsl", true)
	self.brdf_tex_ = root.Texture.from_image(FileSystem:search("images/brdf.png", true), { filter_mode = gl.LINEAR })
	self.volume_ = (self.is_local_) and root.Mesh.build_cube(0.5) or root.Mesh.build_quad()
	self.camera_ = root.Camera(math.radians(90.0), 1.0, vec2(1.0, 100.0))
	self.current_face_ = 1
	self.size_ = 64

	self:rebuild_cubemap()
end

function envprobe:rebuild_cubemap()

    self.cubemap_fbo_ = root.FrameBuffer()
    self.cubemap_albedo_tex_ = root.Texture.from_empty(self.size_, self.size_, { target = gl.TEXTURE_CUBE_MAP, iformat = gl.RGBA16F, type = gl.FLOAT, filter_mode = gl.LINEAR_MIPMAP_LINEAR, mipmap = true })
	self.cubemap_clouds_tex_ = root.Texture.from_empty(self.size_, self.size_, { iformat = gl.RGBA8, filter_mode = gl.LINEAR })
	
	if self.is_local_ then
		self.cubemap_depth_tex_ = root.Texture.from_empty(self.size_, self.size_, { iformat = gl.DEPTH_COMPONENT24, format = gl.DEPTH_COMPONENT })
	end
end

function envprobe:set_size(value)
	self.size_ = value
	self:rebuild_cubemap()
end

function envprobe:parse(object)

    self.size_ = object:get_number("size")
	self:rebuild_cubemap()
	
	object_base.parse(self, object)
end

function envprobe:write(object)

	object:set_number("size", self.size_)
	object_base.write(self, object)
end

function envprobe:generate_reflection()

    gl.PushAttrib(gl.VIEWPORT_BIT)
    gl.Viewport(0, 0, self.size_, self.size_)
	
	for i = 1, 6 do
	
		local volumetric_cloud_pass = function()
		
			self.cubemap_fbo_:clear()
			self.cubemap_fbo_:attach(self.cubemap_clouds_tex_, gl.COLOR_ATTACHMENT0)
			self.cubemap_fbo_:bind_output()
			
			gl.ClearColor(root.Color.Black)
			gl.Clear(gl.COLOR_BUFFER_BIT)
			
			self.camera_.position = vec3()
			self.camera_.rotation = quat.from_axis_cubemap(i - 1)
			self.camera_:refresh()
			
			-- cheap cloud pass here
			self.planet_.clouds_:draw(self.camera_, self.planet_.sun_:get_direction(), 16)
			root.FrameBuffer.unbind()
		end
		
		local sky_pass = function()
		
			self.cubemap_fbo_:clear()
			self.cubemap_fbo_:attach(self.cubemap_albedo_tex_, gl.COLOR_ATTACHMENT0, i - 1)
			self.cubemap_fbo_:bind_output()

			gl.ClearColor(root.Color.Black)
			gl.Clear(gl.COLOR_BUFFER_BIT)
			
			self.camera_.position.y = 1500.0
			self.camera_:refresh()
			
			self.sky_shader_:bind()
			root.Shader.get():uniform("u_InvViewProjMatrix", math.inverse(self.camera_.view_proj_origin_matrix))
			root.Shader.get():uniform("u_LocalWorldPos", self.camera_.position)
			root.Shader.get():uniform("u_SunDirection", self.planet_.sun_:get_direction())
			self.planet_.atmosphere_:bind(1000)
			
			gl.ActiveTexture(gl.TEXTURE0)
			root.Shader.get():sampler("s_Clouds", 0)
			self.cubemap_clouds_tex_:bind()
			
			self.volume_:draw()     
			root.Shader.pop_stack()
		end
		
		local local_pass = function()
		
			self.cubemap_fbo_:clear()
			self.cubemap_fbo_:attach(self.cubemap_depth_tex_, gl.DEPTH_ATTACHMENT)
			self.cubemap_fbo_:attach(self.cubemap_albedo_tex_, gl.COLOR_ATTACHMENT0, i - 1)
			self.cubemap_fbo_:bind_output()
			
			gl.ClearColor(root.Color.Transparent)
			gl.Clear(bit.bor(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT))

			self.camera_.position = self.transform_.position
			self.camera_.rotation = quat.from_axis_cubemap(i - 1)
			if i - 1 == math.PosX or i - 1 == math.NegX then self.camera_.clip.y = self.transform_.scale.x * 0.5 end
			if i - 1 == math.PosY or i - 1 == math.NegY then self.camera_.clip.y = self.transform_.scale.y * 0.5 end
			if i - 1 == math.PosZ or i - 1 == math.NegZ then self.camera_.clip.y = self.transform_.scale.z * 0.5 end
			self.camera_:refresh()
			
			self.gbuffer_shader_:bind()
			root.Shader.get():uniform("u_WorldCamPosition", self.camera_.position)
			root.Shader.get():uniform("u_SunDirection", self.planet_.sun_:get_direction())
			root.Shader.get():uniform("u_DeferredDist", 0.0)
			self.planet_.atmosphere_:bind(1000)
			
			gl.Enable(gl.DEPTH_TEST)
            gl.Enable(gl.CULL_FACE)	
			
			for i = 1, table.getn(self.planet_.active_sectors_) do
				local sector = self.planet_.active_sectors_[i]
				for j = 1, table.getn(sector.objects_) do
					sector.objects_[j]:draw(self.camera_)
				end
			end
			
			gl.Disable(gl.DEPTH_TEST)
            gl.Disable(gl.CULL_FACE)
			root.Shader.pop_stack()
		end
		
		if i == self.current_face_ then
			if not self.is_local_ then
				volumetric_cloud_pass()
				sky_pass()
			else
				local_pass()
			end
		end
	end
	
    self.current_face_ = self.current_face_ + 1
    if self.current_face_ > 6 then
        self.cubemap_albedo_tex_:generate_mipmap()
        self.current_face_ = 1
    end
	
	if not self.is_local_ then
		for i = 1, table.getn(self.planet_.active_sectors_) do
			local sector = self.planet_.active_sectors_[i]
			for j = 1, table.getn(sector.objects_) do
				if sector.objects_[j].type_id_ == ID_ENVPROBE then
					sector.objects_[j]:generate_reflection()
				end
			end
		end
	end
	
	root.FrameBuffer.unbind()
	gl.PopAttrib()
end

function envprobe:draw_light(camera)

	gl.Enable(gl.TEXTURE_CUBE_MAP_SEAMLESS)
	
	gl.ActiveTexture(gl.TEXTURE4)
	root.Shader.get():sampler("s_BRDF", 4)
	self.brdf_tex_:bind()
	
	if not self.is_local_ then
	
		gl.StencilFunc(gl.NOTEQUAL, 2, 0xFF)
		gl.StencilMask(0x00)
		
		gl.ActiveTexture(gl.TEXTURE5)
		root.Shader.get():sampler("s_GlobalCubemap", 5)
		self.cubemap_albedo_tex_:bind()
		
		gl.ActiveTexture(gl.TEXTURE6)
		root.Shader.get():sampler("s_LocalCubemap", 6)
		root.Texture.unbind() -- weird driver bugfix
		
		root.Shader.get():uniform("u_ViewProjMatrix", mat4())
		root.Shader.get():uniform("u_LocalPass", 0.0)
		self.volume_:draw()
	else
	
        gl.StencilFunc(gl.ALWAYS, 2, 0xFF)
        gl.StencilOp(gl.KEEP, gl.KEEP, gl.REPLACE)
        gl.StencilMask(0xFF)
		gl.CullFace(gl.FRONT)
		
		gl.ActiveTexture(gl.TEXTURE5)
		root.Shader.get():sampler("s_GlobalCubemap", 5)
		self.planet_.global_envprobe_.cubemap_albedo_tex_:bind()
		
		gl.ActiveTexture(gl.TEXTURE6)
		root.Shader.get():sampler("s_LocalCubemap", 6)
		self.cubemap_albedo_tex_:bind()
		
		root.Shader.get():uniform("u_LocalCubemapPosition", self.transform_.position)
		root.Shader.get():uniform("u_LocalCubemapExtent", self.transform_.scale * 0.5)
		root.Shader.get():uniform("u_LocalPass", 1.0)
		self.volume_:copy(self.transform_)
		self.volume_:draw(camera)
		
		gl.StencilOp(gl.KEEP, gl.KEEP, gl.REPLACE)
        gl.StencilMask(0x00)
		gl.CullFace(gl.BACK)
	end
	
	gl.Disable(gl.TEXTURE_CUBE_MAP_SEAMLESS)
end