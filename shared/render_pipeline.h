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
	
		root.FrameBuffer.unbind()	
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
		root.Shader.pop_stack()
		gl.Disable(gl.BLEND)
	end
	
	generate_lensflare()
	generate_lensflare_features()
	self:perform_blur(self.gauss1d_shader_, self.lensflare_features_tex_, self.lensflare_tex_)
	apply_lensflare()
end

function render_pipeline:render(dt, camera)

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
        
        self.gbuffer_shader_:bind()
        
        gl.ClearColor(root.Color.Black)
        gl.Clear(bit.bor(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT, gl.STENCIL_BUFFER_BIT))
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
		
			self.render_fbo_:clear()
			self.render_fbo_:attach(self.clouds_tex_, gl.COLOR_ATTACHMENT0)
			self.render_fbo_:bind_output()
			
			gl.PushAttrib(gl.VIEWPORT_BIT)
			gl.Viewport(0, 0, self.clouds_tex_.w, self.clouds_tex_.h)
			gl.ClearColor(root.Color.Transparent)
			gl.Clear(gl.COLOR_BUFFER_BIT)
			
			self.planet_.clouds_:draw(camera, self.planet_.sun_:get_direction(), 64)       
			gl.PopAttrib()
		end

		local deferred_sky_pass = function()
		
			gl.StencilFunc(gl.EQUAL, 0, 0xFF)
			gl.StencilMask(0x00)
		
			self.sky_shader_:bind()
			root.Shader.get():uniform("u_InvViewProjMatrix", math.inverse(camera.view_proj_origin_matrix))
			root.Shader.get():uniform("u_LocalWorldPos", camera.position)
			root.Shader.get():uniform("u_SunDirection", self.planet_.sun_:get_direction())
			self.planet_.atmosphere_:bind(1000)
			
			gl.ActiveTexture(gl.TEXTURE0)
			root.Shader.get():sampler("s_Clouds", 0)
			self.clouds_tex_:bind()
			
			self.screen_quad_:draw()
			root.Shader.pop_stack()
		end
    
        local deferred_lights_pass = function()
            
            gl.BlendFunc(gl.ONE, gl.ONE)
            gl.Enable(gl.BLEND)
            gl.Enable(gl.CULL_FACE)
			
			local light_pass = function(light)
			
				gl.StencilFunc(gl.NOTEQUAL, 0, 0xFF)
				gl.StencilMask(0x00)
			
				light.shader_:bind()
				root.Shader.get():uniform("u_InvViewProjMatrix", math.inverse(camera.view_proj_matrix))
				root.Shader.get():uniform("u_WorldCamPosition", camera.position)
			
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
			
				light:__draw_light(camera)
				root.Shader.pop_stack()
			end
            
			-- draw planet global lights
            for i = 1, table.getn(self.planet_.objects_) do
                if self.planet_.objects_[i].is_light_ then
					light_pass(self.planet_.objects_[i])
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
            
            gl.Disable(gl.BLEND)
            gl.Disable(gl.CULL_FACE)
        end
        
		self.planet_.sun_:generate_shadow(camera)
		self.planet_.global_probe_:generate()
		generate_occlusion()
        generate_volumetric_clouds()
    
        self.render_fbo_:clear()
        self.render_fbo_:attach(self.gbuffer_depth_tex_, gl.DEPTH_STENCIL_ATTACHMENT)
        self.render_fbo_:attach(self.render_tex_, gl.COLOR_ATTACHMENT0)
        self.render_fbo_:bind_output()
        
        gl.ClearColor(root.Color.Black)
        gl.Clear(gl.COLOR_BUFFER_BIT)      
        gl.Enable(gl.STENCIL_TEST)
        
        deferred_sky_pass()
        deferred_lights_pass()
		
        gl.Disable(gl.STENCIL_TEST)  
        root.FrameBuffer.unbind()
		
		self:process_bloom(self.render_tex_)
		root.FrameBuffer.unbind()
    end
    
    local final_pass = function()
	
		local final_tex = self:perform_smaa(self.render_tex_, self.render_smaa_tex_)
		root.FrameBuffer.unbind()   
        self.final_shader_:bind()
		
        root.Shader.get():uniform("u_InvViewProjMatrix", math.inverse(camera.view_proj_matrix))
        root.Shader.get():uniform("u_PrevViewProjMatrix", self.prev_view_proj_matrix_)  
        root.Shader.get():uniform("u_MotionScale", (1.0 / 60.0) / dt)
        self.prev_view_proj_matrix_ = mat4(camera.view_proj_matrix)

        gl.ActiveTexture(gl.TEXTURE0)
        root.Shader.get():sampler("s_Tex0", 0)
        self.gbuffer_depth_tex_:bind()
        
        gl.ActiveTexture(gl.TEXTURE1)
        root.Shader.get():sampler("s_Tex1", 1)
        final_tex:bind()
        
        self.screen_quad_:draw()
        root.Shader.pop_stack()
		
		self:process_lensflare(camera, final_tex)
		root.FrameBuffer.unbind()
    end
    
    gbuffer_pass()
    deferred_pass()
    final_pass()	
end