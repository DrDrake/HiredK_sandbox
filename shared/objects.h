--[[
- @file objects.h
- @brief
]]

--[[
////////////////////////////////////////////////////////////////////////////////
// object_base implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'object_base' (root.ScriptObject)

function object_base:__init(planet)
    root.ScriptObject.__init(self, "object")
    self.type = root.ScriptObject.Dynamic
    self.planet_ = planet
    self.transform_ = root.Transform()
    self.shader_ = nil
    self.body_ = nil
    self.mesh_ = nil
    self.is_light_ = false
end

function object_base:__on_physics_start(dynamic_world)
end

function object_base:__update(dt)

    if self.body_ ~= nil then
        self.transform_.position = self.body_.position
        self.transform_.rotation = self.body_.rotation
    end
end

function object_base:__draw(camera)

	if self.mesh_ ~= nil then
		self.mesh_:copy(self.transform_)
		self.mesh_:draw(camera)
	end
end

function object_base:__draw_shadow(matrix)

    if self.mesh_ ~= nil then
        root.Shader.get():uniform("u_ViewProjMatrix", matrix)
        self.mesh_:copy(self.transform_)
        self.mesh_:draw()
    end
end

function object_base:__draw_light(camera)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// box_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'box_object' (object_base)

function box_object:__init(planet, extent, mass)
    object_base.__init(self, planet)

	self.shader_ = FileSystem:search("shaders/gbuffer_parallax.glsl", true)
    self.mesh_ = root.Mesh.build_box(vec3(-extent.x, -extent.y, -extent.z), extent)
    self.extent_ = extent
    self.mass_ = mass
	
	local tex_settings = { filter_mode = gl.LINEAR_MIPMAP_LINEAR, anisotropy = 16, mipmap = true }
	self.albedo_tex_ = root.Texture.from_image(FileSystem:search("images/rock.png", true), tex_settings)
	self.normal_tex_ = root.Texture.from_image(FileSystem:search("images/rock_n.png", true), tex_settings)	
end

function box_object:__on_physics_start(dynamic_world)
    self.body_ = bt.RigidBody.create_box(dynamic_world, self.transform_.position, self.extent_, self.mass_)
end

function box_object:__draw(camera)

	self.shader_:bind()

	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("s_Albedo", 0)
	self.albedo_tex_:bind()
	
	gl.ActiveTexture(gl.TEXTURE1)
	root.Shader.get():sampler("s_Normal", 1)
	self.normal_tex_:bind()
	
	root.Shader.get():uniform("u_WorldCamPosition", camera.position)
	root.Shader.get():uniform("u_SunDirection", self.planet_.sun_:get_direction())
	
	object_base.__draw(self, camera)
	root.Shader.pop_stack()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// model_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'model_object' (object_base)

function model_object:__init(planet, path, mass)
    object_base.__init(self, planet)
	self.mass_ = mass
	
    self.mesh_ = FileSystem:search(path, true)
    self.transform_.box:set(self.mesh_.box)
end

function model_object:__on_physics_start(dynamic_world)
	self.body_ = bt.RigidBody.create_collider(dynamic_world, self.transform_.position, self.mesh_, self.mass_)
end

function model_object:__draw(camera)

    self.mesh_:copy(self.transform_)
    self.mesh_:draw(camera)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// tree_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'tree_object' (object_base)

function tree_object:__init(planet, path)
    object_base.__init(self, planet)
    
    self.shader_ = FileSystem:search("shaders/gbuffer_tree.glsl", true)
    self.mesh_ = FileSystem:search(path, true)
    self.transform_.box:set(self.mesh_.box)
    self.timer_ = 0.0
end

function tree_object:__update(dt)

    self.timer_ = self.timer_ + dt * 0.01
    object_base.__update(self, dt)
end

function tree_object:__draw(camera)

	self.shader_:bind()
	root.Shader.get():uniform("u_Timer", self.timer_)
	object_base.__draw(self, camera)
	root.Shader.pop_stack()
end

function tree_object:__draw_shadow(matrix)

	self.shader_:bind()
	root.Shader.get():uniform("u_Timer", self.timer_)
	object_base.__draw_shadow(self, matrix)
	root.Shader.pop_stack()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// directional_light implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'directional_light' (object_base)

function directional_light:__init(planet, num_splits, resolution)
    object_base.__init(self, planet)
    
    self.shader_ = FileSystem:search("shaders/pbr_directional.glsl", true)
	self.shadow_shader_ = FileSystem:search("shaders/gbuffer_simple.glsl", true)
    self.volume_ = root.Mesh.build_quad()
	self.num_splits_ = num_splits
	self.resolution_ = resolution
    self.is_light_ = true
	
	self.shadow_splits_ = { 1.0, 20.0, 100.0, 500.0, 1500.0 }
	self.coord_ = vec2(60.0, 20.0)
	
    self.shadow_fbo_ = root.FrameBuffer()
	self.shadow_tex_array_ = root.Texture.from_empty(resolution, resolution, num_splits, { target = gl.TEXTURE_2D_ARRAY, iformat = gl.DEPTH_COMPONENT24, format = gl.DEPTH_COMPONENT })
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
		local sun_proj = math.ortho(-size, size, -size, size, 200, -size)
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
			self.planet_.active_sectors_[i]:__draw_shadow(shadow_matrix)
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

function directional_light:__draw_light(camera)

    root.Shader.get():uniform_mat4_array("u_LightShadowMatrix", self.shadow_matrices_)	
    root.Shader.get():uniform("u_LightShadowSplits", vec4.from_table(self.shadow_splits_clip_space_))
    root.Shader.get():uniform("u_LightShadowResolution", self.resolution_)
    root.Shader.get():uniform("u_LightDirection", self:get_direction())
	root.Shader.get():uniform("u_DeferredDist", self.planet_.deferred_dist_clip_space_)
	self.planet_.atmosphere_:bind(1000)
    self.planet_.clouds_:bind_coverage()
	
    gl.ActiveTexture(gl.TEXTURE4)
    root.Shader.get():sampler("s_LightDepthTex", 4)
    self.shadow_tex_array_:bind()
	
    self.volume_:draw()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// envprobe implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'envprobe' (object_base)

function envprobe:__init(planet, size)
    object_base.__init(self, planet)
    
    self.shader_ = FileSystem:search("shaders/pbr_envprobe.glsl", true)
    self.sky_shader_ = FileSystem:search("shaders/sky.glsl", true)
    self.brdf_tex_ = root.Texture.from_image(FileSystem:search("images/brdf.png", true), { filter_mode = gl.LINEAR })
	self.screen_quad_ = root.Mesh.build_quad()
	
    self.global_fbo_ = root.FrameBuffer()
    self.global_tex_ = root.Texture.from_empty(size, size, { target = gl.TEXTURE_CUBE_MAP, iformat = gl.RGBA16F, type = gl.FLOAT, filter_mode = gl.LINEAR_MIPMAP_LINEAR, mipmap = true })
    self.clouds_tex_ = root.Texture.from_empty(size, size, { iformat = gl.RGBA8, filter_mode = gl.LINEAR })
    self.camera_ = root.Camera(math.radians(90.0), 1.0, vec2(1.0, 15000.0))
	self.local_volume_ = root.Mesh.build_cube(0.5)
	self.local_probes_ = {}
    self.size_ = size
	self.current_face_ = 1
    self.is_light_ = true
end

function envprobe:add_local_probe(position, extent, size, targets)

	local new_probe =
    {
		shader_ = FileSystem:search("shaders/gbuffer_reflection.glsl", true),
		local_fbo_ = root.FrameBuffer(),
		local_depth_tex_ = root.Texture.from_empty(size, size, { iformat = gl.DEPTH_COMPONENT24, format = gl.DEPTH_COMPONENT }),
		local_tex_ = root.Texture.from_empty(size, size, { target = gl.TEXTURE_CUBE_MAP, iformat = gl.RGBA16F, type = gl.FLOAT, filter_mode = gl.LINEAR_MIPMAP_LINEAR, mipmap = true }),
		camera_ = root.Camera(math.radians(90.0), 1.0, vec2(1.0, 100.0)),
        planet_ = self.planet_,
		position_ = position,
		extent_ = extent,		
		targets_ = targets,
		current_face_ = 1,
		
		generate = function(self)
		
			gl.PushAttrib(gl.VIEWPORT_BIT)
            gl.Viewport(0, 0, size, size)
			self.shader_:bind()
			
            gl.Enable(gl.DEPTH_TEST)
            gl.Enable(gl.CULL_FACE)	
			
			root.Shader.get():uniform("u_WorldCamPosition", self.position_)
			root.Shader.get():uniform("u_SunDirection", self.planet_.sun_:get_direction())
			self.planet_.atmosphere_:bind(1000)
			
            for i = 1, 6 do			
                if i == self.current_face_ then
				
                    self.local_fbo_:clear()
					self.local_fbo_:attach(self.local_depth_tex_, gl.DEPTH_ATTACHMENT)
                    self.local_fbo_:attach(self.local_tex_, gl.COLOR_ATTACHMENT0, i - 1)
                    self.local_fbo_:bind_output()
					
					gl.ClearColor(root.Color.Transparent)
					gl.Clear(bit.bor(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT))
					
                    self.camera_.position = self.position_
                    self.camera_.rotation = quat.from_axis_cubemap(i - 1)
                    self.camera_:refresh()
					
					for i = 1, table.getn(targets) do
						targets[i].mesh_:draw(self.camera_)
					end
				end
			end
			
            self.current_face_ = self.current_face_ + 1
            if self.current_face_ > 6 then
				self.local_tex_:generate_mipmap()
				self.current_face_ = 1
            end
			
            gl.Disable(gl.DEPTH_TEST)
            gl.Disable(gl.CULL_FACE)
			root.FrameBuffer.unbind()
            root.Shader.pop_stack()
            gl.PopAttrib()
		end
	}
	
	table.insert(self.local_probes_, new_probe)
end

function envprobe:generate()

    gl.PushAttrib(gl.VIEWPORT_BIT)
    gl.Viewport(0, 0, self.size_, self.size_)
    
    for i = 1, 6 do    
        if i == self.current_face_ then
		
			local volumetric_cloud_pass = function()
			
				self.global_fbo_:clear()
				self.global_fbo_:attach(self.clouds_tex_, gl.COLOR_ATTACHMENT0)
				self.global_fbo_:bind_output()
				
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
			
				self.global_fbo_:clear()
				self.global_fbo_:attach(self.global_tex_, gl.COLOR_ATTACHMENT0, i - 1)
				self.global_fbo_:bind_output()

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
				self.clouds_tex_:bind()
				
				self.screen_quad_:draw()     
				root.Shader.pop_stack()
			end

            volumetric_cloud_pass()
			sky_pass()
            break
        end
    end
    
    self.current_face_ = self.current_face_ + 1
    if self.current_face_ > 6 then
        self.global_tex_:generate_mipmap()
        self.current_face_ = 1
    end
	
    for i = 1, table.getn(self.local_probes_) do
        self.local_probes_[i]:generate()
    end
    
	root.FrameBuffer.unbind()
	gl.PopAttrib()
end

function envprobe:__draw_light(camera)

	local local_probes_pass = function()
	
		gl.StencilOp(gl.KEEP, gl.KEEP, gl.INCR)
        gl.StencilMask(0xFF)
		gl.CullFace(gl.FRONT)
		
		for i = 1, table.getn(self.local_probes_) do
		
			local offset = self.local_probes_[i].targets_[1].transform_.position + vec3(15, 0, 25)	
			root.Shader.get():uniform("u_LocalCubemapPosition", self.local_probes_[i].position_ - offset)
			root.Shader.get():uniform("u_LocalCubemapExtent", self.local_probes_[i].extent_)
			root.Shader.get():uniform("u_LocalCubemapOffset", offset)
			root.Shader.get():uniform("u_LocalPass", 1.0)
			
			gl.ActiveTexture(gl.TEXTURE6)
			root.Shader.get():sampler("s_LocalCubemap", 6)
			self.local_probes_[i].local_tex_:bind()
			
			self.local_volume_.position = self.local_probes_[i].position_
			self.local_volume_.scale = self.local_probes_[i].extent_ * 2
			self.local_volume_:draw(camera)
		end
		
		gl.StencilOp(gl.KEEP, gl.KEEP, gl.REPLACE)
        gl.StencilMask(0x00)
		gl.CullFace(gl.BACK)
	end

	local global_probe_pass = function()
	
		gl.StencilFunc(gl.NOTEQUAL, 2, 0xFF)
		gl.StencilMask(0x00)
	
		local offset = self.local_probes_[1].targets_[1].transform_.position + vec3(15, 0, 25)	
		root.Shader.get():uniform("u_LocalCubemapPosition", self.local_probes_[1].position_ - offset)
		root.Shader.get():uniform("u_LocalCubemapExtent", self.local_probes_[1].extent_)
		root.Shader.get():uniform("u_LocalCubemapOffset", offset)
		root.Shader.get():uniform("u_LocalPass", 0.0)
		root.Shader.get():uniform("u_ViewProjMatrix", mat4())
		self.screen_quad_:draw()
	end
	
    gl.ActiveTexture(gl.TEXTURE4)
    root.Shader.get():sampler("s_GlobalCubemap", 4)
    self.global_tex_:bind()
	
    gl.ActiveTexture(gl.TEXTURE5)
    root.Shader.get():sampler("s_BRDF", 5)
    self.brdf_tex_:bind()
	
	gl.Enable(gl.TEXTURE_CUBE_MAP_SEAMLESS)
	local_probes_pass()
	global_probe_pass()
	gl.Disable(gl.TEXTURE_CUBE_MAP_SEAMLESS)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// point_light implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'point_light' (object_base)

function point_light:__init(planet, radius)
    object_base.__init(self, planet)

	self.shader_ = FileSystem:search("shaders/pbr_pointlight.glsl", true)
    self.volume_ = root.Mesh.build_cube(radius)
    self.radius_ = radius
	self.color_ = root.Color.White
    self.is_light_ = true
end

function point_light:__draw_light(camera)

    root.Shader.get():uniform("u_InvLightRadius", 1.0 / self.radius_)
    root.Shader.get():uniform("u_LightIntensity", 50.0)
	root.Shader.get():uniform("u_LightWorldPos", self.transform_.position)
	root.Shader.get():uniform("u_LightColor", self.color_)
	gl.CullFace(gl.FRONT)
	
    self.volume_:copy(self.transform_)
    self.volume_:draw(camera)
	gl.CullFace(gl.BACK)
end