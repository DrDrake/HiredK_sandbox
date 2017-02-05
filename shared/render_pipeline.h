--[[
- @file render_pipeline.h
- @brief
]]

class 'render_pipeline' (root.ScriptObject)

function render_pipeline:__init(planet)
    root.ScriptObject.__init(self, "render_pipeline")
    self.planet_ = planet
	
    self.gbuffer_shader_ = FileSystem:search("shaders/gbuffer.glsl", true)
    self.sky_shader_ = FileSystem:search("shaders/sky.glsl", true)
    self.gauss1d_shader_ = FileSystem:search("shaders/gauss1d.glsl", true)
    self.scalebias_shader_ = FileSystem:search("shaders/scalebias.glsl", true)
	self.final_shader_ = FileSystem:search("shaders/final.glsl", true)
	self.screen_quad_ = root.Mesh.build_quad()
	
	self.prev_view_proj_matrix_ = mat4()
	self.local_camera_ = root.Camera()
	
    -- occlusion
    self.bilateral_shader_ = FileSystem:search("shaders/bilateral.glsl", true)
    self.occlusion_shader_ = FileSystem:search("shaders/occlusion.glsl", true)
	self.occlusion_rand_tex_ = root.Texture.from_random(4, 4)
	
	-- smaa
	self.smaa_area_tex_ = root.Texture.from_raw(160, 560, FileSystem:search("images/area.raw", true), { iformat = gl.RG8, format = gl.RG, filter_mode = gl.LINEAR })				
	self.smaa_search_tex_ = root.Texture.from_raw(66, 33, FileSystem:search("images/search.raw", true), { iformat = gl.R8, format = gl.RED })	
	self.smaa_edge_shader_ = FileSystem:search("shaders/smaa/edge.glsl", true)
	self.smaa_blend_shader_ = FileSystem:search("shaders/smaa/blend.glsl", true)
	self.smaa_final_shader_ = FileSystem:search("shaders/smaa/final.glsl", true)
	
	-- lensflare
	self.lensflare_color_tex_ = root.Texture.from_image(FileSystem:search("images/lenscolor.png", true), { target = gl.TEXTURE_1D, filter_mode = gl.LINEAR, wrap_mode = gl.REPEAT })
	self.lensflare_dirt_tex_ = root.Texture.from_image(FileSystem:search("images/lensdirt.png", true), { filter_mode = gl.LINEAR })
	self.lensflare_star_tex_ = root.Texture.from_image(FileSystem:search("images/lensstar.png", true), { filter_mode = gl.LINEAR })
	self.lensflare_features_shader_ = FileSystem:search("shaders/lensflare/features.glsl", true)
	self.lensflare_final_shader_ = FileSystem:search("shaders/lensflare/final.glsl", true)
end

function render_pipeline:rebuild(w, h)

	if self.gbuffer_depth_tex_ and self.gbuffer_depth_tex_.w == w and self.gbuffer_depth_tex_.h == h then
		return
	end

    self.gbuffer_fbo_ = root.FrameBuffer()
    self.gbuffer_depth_tex_ = root.Texture.from_empty(w, h, { iformat = gl.DEPTH32F_STENCIL8, format = gl.DEPTH_COMPONENT, type = gl.FLOAT })
    self.gbuffer_albedo_tex_ = root.Texture.from_empty(w, h)
    self.gbuffer_normal_tex_ = root.Texture.from_empty(w, h)
	
	self.render_fbo_ = root.FrameBuffer()
	self.render_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.render_smaa_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.clouds_tex_ = root.Texture.from_empty(w / 2, h / 2, { iformat = gl.RGBA8, filter_mode = gl.LINEAR })
	self.bloom_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.bloom_blur_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.final_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
	
	-- occlusion
	self.occlusion_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RG16F, format = gl.RG, type = gl.FLOAT })
	self.occlusion_blur_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RG16F, format = gl.RG, type = gl.FLOAT })
    self.bilateral_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
	self.occlusion_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
	
	-- smaa
	self.smaa_edge_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGBA16F, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.smaa_blend_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGBA16F, type = gl.FLOAT, filter_mode = gl.LINEAR })	
	self.smaa_edge_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
	self.smaa_blend_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
	self.smaa_final_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
	
	-- lensflare
	self.lensflare_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGBA16F, format = gl.RGBA, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.lensflare_features_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGBA16F, format = gl.RGBA, type = gl.FLOAT, filter_mode = gl.LINEAR })
end

function render_pipeline:unproject(camera, x, y)

	local viewport = vec4()
	viewport.z = self.gbuffer_depth_tex_.w
	viewport.w = self.gbuffer_depth_tex_.h

	local depth = self.gbuffer_fbo_:read_pixel(self.gbuffer_depth_tex_, gl.DEPTH_STENCIL_ATTACHMENT, x, y).x
    local pos = math.unproject(vec3(x, y, depth), camera.view_matrix, camera.proj_matrix, viewport)
	root.FrameBuffer.unbind()
	return pos
end

function render_pipeline:perform_blur(shader, input_tex, blur_tex)
	
	local blur_pass = function(tex_a, tex_b, direction)
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(tex_a, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		gl.PushAttrib(gl.VIEWPORT_BIT)
		gl.Viewport(0, 0, tex_a.w, tex_a.h)	
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		tex_b:bind()
		
		root.Shader.get():uniform("u_BlurDirection", direction)
		self.screen_quad_:draw()
		gl.PopAttrib()
	end
	
	shader:bind()
	blur_pass(blur_tex, input_tex, vec2(1.0, 0.0))
	blur_pass(input_tex, blur_tex, vec2(0.0, 1.0))
	root.Shader.pop_stack()
	
	return input_tex
end

function render_pipeline:perform_smaa(input_tex, output_tex)

	local edge_pass = function()
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.smaa_edge_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
        
        gl.ClearColor(root.Color.Transparent)
		gl.Clear(gl.COLOR_BUFFER_BIT)
		
		self.smaa_edge_shader_:bind()	
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		input_tex:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
	end
	
	local blend_pass = function()
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.smaa_blend_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		self.smaa_blend_shader_:bind()
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		self.smaa_edge_tex_:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("s_Tex1", 1)
		self.smaa_area_tex_:bind()
		
		gl.ActiveTexture(gl.TEXTURE2)
		root.Shader.get():sampler("s_Tex2", 2)
		self.smaa_search_tex_:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
	end
	
	local final_pass = function()
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(output_tex, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		self.smaa_final_shader_:bind()
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		input_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("s_Tex1", 1)
		self.smaa_blend_tex_:bind()
        
		self.screen_quad_:draw()	
		root.Shader.pop_stack()
	end
	
	edge_pass()
	blend_pass()
	final_pass()
    
    root.FrameBuffer.unbind()
	return output_tex
end

function render_pipeline:process_bloom(input_tex)

	local generate_bloom = function()
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.bloom_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
	
		gl.PushAttrib(gl.VIEWPORT_BIT)
		gl.Viewport(0, 0, self.bloom_tex_.w, self.bloom_tex_.h)	
		self.scalebias_shader_:bind()
		
		root.Shader.get():uniform("u_Scale", vec4(0.2))
		root.Shader.get():uniform("u_Bias", vec4(-1.0))
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		input_tex:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
		gl.PopAttrib()
	end
	
	local apply_bloom = function()
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(input_tex, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		gl.PushAttrib(gl.VIEWPORT_BIT)
		gl.Viewport(0, 0, input_tex.w, input_tex.h)	
		self.scalebias_shader_:bind()
		
		gl.BlendFunc(gl.ONE, gl.ONE)
        gl.Enable(gl.BLEND)
		
		root.Shader.get():uniform("u_Scale", vec4(1.0))
		root.Shader.get():uniform("u_Bias", vec4(0.0))
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		self.bloom_tex_:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
        
        gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
		gl.Disable(gl.BLEND)
		gl.PopAttrib()
	end

    generate_bloom()
    self:perform_blur(self.gauss1d_shader_, self.bloom_tex_, self.bloom_blur_tex_)
    apply_bloom()
	
	return input_tex
end

function render_pipeline:process_lensflare(camera, input_tex)

	local generate_lensflare = function()
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.lensflare_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		gl.PushAttrib(gl.VIEWPORT_BIT)
		gl.Viewport(0, 0, self.lensflare_tex_.w, self.lensflare_tex_.h)	
		self.scalebias_shader_:bind()
		
		root.Shader.get():uniform("u_Scale", vec4(0.2))
		root.Shader.get():uniform("u_Bias", vec4(-3.5))
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		input_tex:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
		gl.PopAttrib()
	end
	
	local generate_lensflare_features = function()
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.lensflare_features_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		gl.PushAttrib(gl.VIEWPORT_BIT)
		gl.Viewport(0, 0, self.lensflare_features_tex_.w, self.lensflare_features_tex_.h)	
		self.lensflare_features_shader_:bind()
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		self.lensflare_tex_:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("s_Tex1", 1)
		self.lensflare_color_tex_:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
		gl.PopAttrib()
	end
    
	local apply_lensflare = function()
	
        self.render_fbo_:clear()
        self.render_fbo_:attach(input_tex, gl.COLOR_ATTACHMENT0)
        self.render_fbo_:bind_output()
		
	    gl.BlendFunc(gl.ONE, gl.ONE)
		gl.Enable(gl.BLEND)
		
		self.lensflare_final_shader_:bind()
		
		local camx = vec3(math.column(camera.view_matrix, 0))
		local camz = vec3(math.column(camera.view_matrix, 2))	
		local rot = math.dot(camx, vec3(1, 0, 0)) * math.dot(camz, vec3(0, 0, 1))
		rot = rot * 4.0
		
		local rot_matrix = math.translate(mat4(), vec3(0.5, 0.5, 0.0))	
		rot_matrix = math.rotate(rot_matrix, rot, vec3(0.0, 0.0, 1.0))
		rot_matrix = math.translate(rot_matrix, vec3(-0.5, -0.5, 0.0))

        root.Shader.get():uniform("u_LensStarMatrix", rot_matrix)		
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		self.lensflare_features_tex_:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("s_Tex1", 1)
		self.lensflare_dirt_tex_:bind()
		
		gl.ActiveTexture(gl.TEXTURE2)
		root.Shader.get():sampler("s_Tex2", 2)
		self.lensflare_star_tex_:bind()
		
		self.screen_quad_:draw()
		gl.Disable(gl.BLEND)
		
		root.Shader.pop_stack()
		root.FrameBuffer.unbind()
	end
	
	generate_lensflare()
	generate_lensflare_features()
	self:perform_blur(self.gauss1d_shader_, self.lensflare_features_tex_, self.lensflare_tex_)
	apply_lensflare()
end

function render_pipeline:render(dt, camera)

	self.local_camera_:copy(camera)
	self.local_camera_.position = camera.position - vec3(0, 6360000, 0)
	self.local_camera_:refresh()

    local gbuffer_pass = function()

        self.gbuffer_fbo_:clear()
        self.gbuffer_fbo_:attach(self.gbuffer_depth_tex_, gl.DEPTH_STENCIL_ATTACHMENT)
        self.gbuffer_fbo_:attach(self.gbuffer_albedo_tex_, gl.COLOR_ATTACHMENT0)
        self.gbuffer_fbo_:attach(self.gbuffer_normal_tex_, gl.COLOR_ATTACHMENT1)
        self.gbuffer_fbo_:bind_output()
        
        gl.Enable(gl.DEPTH_TEST)
        gl.Enable(gl.STENCIL_TEST)
        gl.Enable(gl.CULL_FACE)
        
        gl.StencilFunc(gl.ALWAYS, 1, 0xFF)
        gl.StencilOp(gl.KEEP, gl.KEEP, gl.REPLACE)
        gl.StencilMask(0xFF)
        
        gl.ClearColor(root.Color.Black)
        gl.Clear(bit.bor(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT, gl.STENCIL_BUFFER_BIT))
		
		self.gbuffer_shader_:bind()
        self.planet_:draw(camera)
		root.Shader.pop_stack()
  
        gl.Disable(gl.DEPTH_TEST)
        gl.Disable(gl.STENCIL_TEST)
        gl.Disable(gl.CULL_FACE)        
        root.FrameBuffer.unbind()
    end
	
	local deferred_pass = function()
	
         local generate_occlusion = function()

            self.render_fbo_:clear()
            self.render_fbo_:attach(self.occlusion_tex_, gl.COLOR_ATTACHMENT0)
            self.render_fbo_:bind_output()
            
            gl.ClearColor(root.Color.Black)
            gl.Clear(gl.COLOR_BUFFER_BIT)   
            self.occlusion_shader_:bind()

            local focal_len = vec2()
            focal_len.x = 1.0 / math.tan(camera.fov * 0.5) * (self.occlusion_tex_.h / self.occlusion_tex_.w)
            focal_len.y = 1.0 / math.tan(camera.fov * 0.5)
            
            local lin_mad = vec2()
            lin_mad.x = (camera.clip.x - camera.clip.y) / (2.0 * camera.clip.x * camera.clip.y)
            lin_mad.y = (camera.clip.x + camera.clip.y) / (2.0 * camera.clip.x * camera.clip.y)
            
            root.Shader.get():uniform("u_UVToViewA", -2.0 * (1.0 / focal_len.x), -2.0 * (1.0 / focal_len.y))
            root.Shader.get():uniform("u_UVToViewB", 1.0 / focal_len.x, 1.0 / focal_len.y)
            root.Shader.get():uniform("u_FocalLen", focal_len)
            root.Shader.get():uniform("u_LinMAD", lin_mad)
            
            gl.ActiveTexture(gl.TEXTURE0)
            root.Shader.get():sampler("s_Tex0", 0)
            self.gbuffer_depth_tex_:bind()
            
            gl.ActiveTexture(gl.TEXTURE1)
            root.Shader.get():sampler("s_Tex1", 1)
            self.occlusion_rand_tex_:bind()
            
            self.screen_quad_:draw()
            root.Shader.pop_stack()
            
            self:perform_blur(self.bilateral_shader_, self.occlusion_tex_, self.occlusion_blur_tex_)
            root.FrameBuffer.unbind()
        end
	
		local generate_volumetric_clouds = function()
		
			if not camera.ortho then
				self.render_fbo_:clear()
				self.render_fbo_:attach(self.clouds_tex_, gl.COLOR_ATTACHMENT0)
				self.render_fbo_:bind_output()
				
				gl.PushAttrib(gl.VIEWPORT_BIT)
				gl.Viewport(0, 0, self.clouds_tex_.w, self.clouds_tex_.h)
				gl.ClearColor(root.Color.Transparent)
				gl.Clear(gl.COLOR_BUFFER_BIT)
				
				local local_camera = self.planet_.spherical_ and self.local_camera_ or camera
				self.planet_.clouds_:draw(local_camera, self.planet_.sun_:get_direction(), 64)       
				gl.PopAttrib()
			end
		end
		
		local deferred_sky_pass = function()
		
			if not camera.ortho then
			
				gl.Enable(gl.STENCIL_TEST)
				gl.StencilFunc(gl.EQUAL, 0, 0xFF)
				gl.StencilMask(0x00)
				
				self.sky_shader_:bind()
				local local_camera = self.planet_.spherical_ and self.local_camera_ or camera
				root.Shader.get():uniform("u_InvViewProjMatrix", math.inverse(local_camera.view_proj_origin_matrix))
				root.Shader.get():uniform("u_LocalWorldPos", local_camera.position)
				root.Shader.get():uniform("u_SunDirection", self.planet_.sun_:get_direction())
				self.planet_.atmosphere_:bind(1000)
				
				gl.ActiveTexture(gl.TEXTURE0)
				root.Shader.get():sampler("s_Clouds", 0)
				self.clouds_tex_:bind()
				
				self.screen_quad_:draw()
				root.Shader.pop_stack()
				
				gl.Disable(gl.STENCIL_TEST)
			end
		end
		
		local deferred_transparent_pass = function()
		
			gl.Enable(gl.BLEND)
			gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
			gl.Enable(gl.DEPTH_TEST)	
			
			-- draw sectors transparent objects
			local local_camera = self.planet_.spherical_ and self.local_camera_ or camera
			for i = 1, table.getn(self.planet_.active_sectors_) do
				local sector = self.planet_.active_sectors_[i]
				for j = 1, table.getn(sector.objects_) do
					 sector.objects_[j]:draw_transparent(local_camera)
				end
			end
			
			gl.Disable(gl.DEPTH_TEST)
			gl.Disable(gl.STENCIL_TEST)
			gl.Disable(gl.BLEND)
		end
		
        local deferred_lights_pass = function()
            
			gl.Enable(gl.BLEND)
            gl.BlendFunc(gl.ONE, gl.ONE)
            gl.Enable(gl.CULL_FACE)
			
			gl.Enable(gl.STENCIL_TEST)
			gl.StencilFunc(gl.NOTEQUAL, 0, 0xFF)
			gl.StencilMask(0x00)
			
			local light_pass = function(light)
				
				light.shader_:bind()
				
				local local_camera = self.planet_.spherical_ and self.local_camera_ or camera			
				root.Shader.get():uniform("u_InvViewProjMatrix", math.inverse(local_camera.view_proj_matrix))
				root.Shader.get():uniform("u_WorldCamPosition", local_camera.position)
			
				gl.ActiveTexture(gl.TEXTURE0)
				root.Shader.get():sampler("s_Tex0", 0)
				self.gbuffer_depth_tex_:bind()
				
				gl.ActiveTexture(gl.TEXTURE1)
				root.Shader.get():sampler("s_Tex1", 1)
				self.gbuffer_albedo_tex_:bind()
				
				gl.ActiveTexture(gl.TEXTURE2)
				root.Shader.get():sampler("s_Tex2", 2)
				self.gbuffer_normal_tex_:bind()
				
				gl.ActiveTexture(gl.TEXTURE3)
				root.Shader.get():sampler("s_Tex3", 3)
				self.occlusion_tex_:bind()
			
				light:draw_light(local_camera)
				root.Shader.pop_stack()
			end
            
			-- draw planet global lights (skip global envprobe)
            for i = 1, table.getn(self.planet_.objects_) do
				local object = self.planet_.objects_[i]
                if object.is_light_ and object.type_id_ ~= ID_ENVPROBE then
					light_pass(object)
                end
            end
			
			-- draw sectors local lights
			for i = 1, table.getn(self.planet_.active_sectors_) do
				local sector = self.planet_.active_sectors_[i]
				for j = 1, table.getn(sector.objects_) do
					 if sector.objects_[j].is_light_ then
						light_pass(sector.objects_[j])
					 end
				end
			end
			
			-- draw global envprobe last
			light_pass(self.planet_.global_envprobe_)
			
            gl.Disable(gl.CULL_FACE)
			gl.Disable(gl.STENCIL_TEST)
			gl.Disable(gl.BLEND)
        end
		
		local local_camera = self.planet_.spherical_ and self.local_camera_ or camera
		self.planet_.sun_:generate_shadow(local_camera)
		generate_occlusion()
		generate_volumetric_clouds()
		
        self.render_fbo_:clear()
        self.render_fbo_:attach(self.gbuffer_depth_tex_, gl.DEPTH_STENCIL_ATTACHMENT)
        self.render_fbo_:attach(self.render_tex_, gl.COLOR_ATTACHMENT0)
        self.render_fbo_:bind_output()
		
        gl.ClearColor(root.Color.Black)
        gl.Clear(gl.COLOR_BUFFER_BIT)      

		deferred_sky_pass()
		deferred_transparent_pass()
		deferred_lights_pass()
	
        gl.Disable(gl.STENCIL_TEST)
		
		self:process_bloom(self.render_tex_)
        root.FrameBuffer.unbind()
	end
	
    local final_pass = function()

		local final_tex = self:perform_smaa(self.render_tex_, self.render_smaa_tex_)
		
        self.render_fbo_:clear()
        self.render_fbo_:attach(self.final_tex_, gl.COLOR_ATTACHMENT0)
        self.render_fbo_:bind_output()
		
		local local_camera = self.planet_.spherical_ and self.local_camera_ or camera
		if not self.planet_.enable_logz_ then
		
			self.final_shader_:bind()		
			root.Shader.get():uniform("u_InvViewProjMatrix", math.inverse(local_camera.view_proj_matrix))
			root.Shader.get():uniform("u_PrevViewProjMatrix", self.prev_view_proj_matrix_)  
			root.Shader.get():uniform("u_MotionScale", (1.0 / 60.0) / dt)
			self.prev_view_proj_matrix_ = mat4(local_camera.view_proj_matrix)
			
			gl.ActiveTexture(gl.TEXTURE0)
			root.Shader.get():sampler("s_Tex0", 0)
			self.gbuffer_depth_tex_:bind()
			
			gl.ActiveTexture(gl.TEXTURE1)
			root.Shader.get():sampler("s_Tex1", 1)
			final_tex:bind()
			
			self.screen_quad_:draw()
			root.Shader.pop_stack()
		else	
		
			gl.ActiveTexture(gl.TEXTURE0)
			root.Shader.get():uniform("u_RenderingType", 2)
			root.Shader.get():sampler("s_Tex0", 0)
			final_tex:bind()
			
			self.screen_quad_:draw()
		end
		
		self:process_lensflare(local_camera, self.final_tex_)
		root.FrameBuffer.unbind()
    end
	
	gbuffer_pass()
	deferred_pass()
	final_pass()

	return self.final_tex_
end