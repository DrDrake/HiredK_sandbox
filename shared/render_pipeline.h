--[[
- @file render_pipeline.h
- @brief
]]

class 'render_pipeline' (root.ScriptObject)

function render_pipeline:__init(scene)
    root.ScriptObject.__init(self, "render_pipeline")
    self.type = root.ScriptObject.Dynamic
    self.scene_ = scene
    
    self.bilateral_shader_ = FileSystem:search("shaders/bilateral.glsl", true)
    self.gauss1d_shader_ = FileSystem:search("shaders/gauss1d.glsl", true)
    self.scalebias_shader_ = FileSystem:search("shaders/scalebias.glsl", true)
    self.sky_shader_ = FileSystem:search("shaders/sky.glsl", true)
    self.light_deferred_shader_ = FileSystem:search("shaders/light_deferred.glsl", true)
    self.final_shader_ = FileSystem:search("shaders/final.glsl", true)
    self.screen_quad_ = root.Mesh.build_quad()  
    self.motion_scale_ = 1.0 / 60.0
    
    self.enable_shadow_ = true
    self.enable_bloom_ = true
    self.enable_occlusion_ = true
    self.enable_lensflare_ = true
    self.enable_reflection_ = true
    
    -- occlusion
	self.hbao_shader_ = FileSystem:search("shaders/hbao.glsl", true)
	self.hbao_rand_tex_ = root.Texture.from_random(4, 4)
    
	-- lensflare
	self.lensflare_color_tex_ = root.Texture.from_image(FileSystem:search("lenscolor.png", true), { target = gl.TEXTURE_1D, filter_mode = gl.LINEAR, wrap_mode = gl.REPEAT })
	self.lensflare_dirt_tex_ = root.Texture.from_image(FileSystem:search("lensdirt.png", true), { filter_mode = gl.LINEAR })
	self.lensflare_star_tex_ = root.Texture.from_image(FileSystem:search("lensstar.png", true), { filter_mode = gl.LINEAR })
	self.lensflare_shader_ = FileSystem:search("shaders/lensflare.glsl", true)
    
	-- smaa
	self.smaa_area_tex_ = root.Texture.from_raw(160, 560, FileSystem:search("smaa_area.raw", true), { iformat = gl.RG8, format = gl.RG, filter_mode = gl.LINEAR })				
	self.smaa_search_tex_ = root.Texture.from_raw(66, 33, FileSystem:search("smaa_search.raw", true), { iformat = gl.R8, format = gl.RED })	
	self.smaa_edge_shader_ = FileSystem:search("shaders/smaa/edge.glsl", true)
	self.smaa_blend_shader_ = FileSystem:search("shaders/smaa/blend.glsl", true)
	self.smaa_final_shader_ = FileSystem:search("shaders/smaa/final.glsl", true)
end

function render_pipeline:rebuild(w, h)

    if self.gbuffer_depth_tex_ ~= nil and self.gbuffer_depth_tex_.w == w and self.gbuffer_depth_tex_.h == h then
        return
    end

    self.gbuffer_fbo_ = root.FrameBuffer()
    self.gbuffer_depth_tex_ = root.Texture.from_empty(w, h, { iformat = gl.DEPTH_COMPONENT32F, format = gl.DEPTH_COMPONENT, type = gl.FLOAT })
    self.gbuffer_geometric_tex_ = root.Texture.from_empty(w, h)
    self.gbuffer_diffuse_tex_ = root.Texture.from_empty(w, h)
    
    self.render_fbo_ = root.FrameBuffer()
    self.render_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
    self.render_smaa_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
    self.final_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
    self.bilateral_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
    
    self.clouds_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGBA8, filter_mode = gl.LINEAR })
    
	-- occlusion
	self.occlusion_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RG16F, format = gl.RG, type = gl.FLOAT })
	self.occlusion_blur_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RG16F, format = gl.RG, type = gl.FLOAT })
	self.hbao_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
    
    -- bloom
	self.bloom_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.bloom_blur_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
    
    -- lensflare
	self.lensflare_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGBA16F, format = gl.RGBA, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.lensflare_features_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGBA16F, format = gl.RGBA, type = gl.FLOAT, filter_mode = gl.LINEAR })
    
	-- smaa
    self.smaa_fbo_ = root.FrameBuffer()
	self.smaa_edge_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGBA16F, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.smaa_blend_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGBA16F, type = gl.FLOAT, filter_mode = gl.LINEAR })	
	self.smaa_edge_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
	self.smaa_blend_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
	self.smaa_final_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
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

function render_pipeline:process_occlusion(camera)

	local generate_occlusion = function()
	
		self.hbao_shader_:bind()
	
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
		self.hbao_rand_tex_:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
	end
    
    self.render_fbo_:clear()
    self.render_fbo_:attach(self.occlusion_tex_, gl.COLOR_ATTACHMENT0)
    self.render_fbo_:bind_output()
    
    gl.ClearColor(root.Color.Black)
    gl.Clear(gl.COLOR_BUFFER_BIT)	
	
    if self.enable_occlusion_ then
        generate_occlusion()
        self:perform_blur(self.bilateral_shader_, self.occlusion_tex_, self.occlusion_blur_tex_)
    end
    
    root.FrameBuffer.unbind()
	return self.occlusion_tex_
end

function render_pipeline:create_hdr_render(planet_camera, sector_camera, occlusion_tex)

	local generate_volumetric_clouds = function()
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.clouds_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		gl.PushAttrib(gl.VIEWPORT_BIT)
		gl.Viewport(0, 0, self.clouds_tex_.w, self.clouds_tex_.h)
		gl.ClearColor(root.Color.Transparent)
		gl.Clear(gl.COLOR_BUFFER_BIT)
        
        self.scene_.planet_.clouds_:draw(sector_camera, 256)       
		gl.PopAttrib()
	end

	local draw_sky = function()
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.render_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
        
        if planet_camera.ortho then   
            gl.ClearColor(root.Color(100, 100, 100, 255))
            gl.Clear(gl.COLOR_BUFFER_BIT)
            return
        end
		
		gl.ClearColor(root.Color.Black)
		gl.Clear(gl.COLOR_BUFFER_BIT)        
		self.sky_shader_:bind()
        
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Clouds", 0)
		self.clouds_tex_:bind()
        
		root.Shader.get():uniform("u_InvProjMatrix", math.inverse(planet_camera.proj_matrix))
		root.Shader.get():uniform("u_InvViewMatrix", math.inverse(planet_camera.view_matrix))  
        
        root.Shader.get():uniform("u_CameraPos", planet_camera.position)
        root.Shader.get():uniform("u_SunDir", self.scene_.sun_:get_direction())   
		self.scene_.planet_.atmosphere_:bind(1000.0)
        
		self.screen_quad_:draw()
		root.Shader.pop_stack()
	end
	
	local deferred_light_pass = function()
	
		self.light_deferred_shader_:bind()	
        gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
		gl.Enable(gl.BLEND)
        
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		self.gbuffer_depth_tex_:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("s_Tex1", 1)
		self.gbuffer_diffuse_tex_:bind()
        
        gl.ActiveTexture(gl.TEXTURE2)
		root.Shader.get():sampler("s_Tex2", 2)
		self.gbuffer_geometric_tex_:bind()
        
        gl.ActiveTexture(gl.TEXTURE3)
		root.Shader.get():sampler("s_Tex3", 3)
		occlusion_tex:bind()
        
        root.Shader.get():uniform("u_InvViewProjMatrix", math.inverse(sector_camera.view_proj_matrix))
        root.Shader.get():uniform("u_SectorCameraPos", sector_camera.position)   
        root.Shader.get():uniform("u_EnableShadow", self.enable_shadow_ and 1 or 0)
        self.scene_:bind_global(planet_camera)
        
		self.screen_quad_:draw()
		root.Shader.pop_stack()
        gl.Disable(gl.TEXTURE_CUBE_MAP_SEAMLESS)
		gl.Disable(gl.BLEND)
	end

    generate_volumetric_clouds()
	draw_sky()
	deferred_light_pass()
    
    root.FrameBuffer.unbind()
	return self.render_tex_
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

    if self.enable_bloom_ then
    
        generate_bloom()
        self:perform_blur(self.gauss1d_shader_, self.bloom_tex_, self.bloom_blur_tex_)
        apply_bloom()
    end
    
    root.FrameBuffer.unbind()
	return input_tex
end

function render_pipeline:process_lensflare(input_tex)

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
		self.lensflare_shader_:bind()
		
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
    
    if self.enable_lensflare_ then 

        generate_lensflare()
        generate_lensflare_features()
        self:perform_blur(self.gauss1d_shader_, self.lensflare_features_tex_, self.lensflare_tex_)
    end
    
    root.FrameBuffer.unbind()
	return input_tex
end

function render_pipeline:perform_smaa(input_tex, output_tex)

	local edge_pass = function()
	
		self.smaa_fbo_:clear()
		self.smaa_fbo_:attach(self.smaa_edge_tex_, gl.COLOR_ATTACHMENT0)
		self.smaa_fbo_:bind_output()
        
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
	
		self.smaa_fbo_:clear()
		self.smaa_fbo_:attach(self.smaa_blend_tex_, gl.COLOR_ATTACHMENT0)
		self.smaa_fbo_:bind_output()
		
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
	
		self.smaa_fbo_:clear()
		self.smaa_fbo_:attach(output_tex, gl.COLOR_ATTACHMENT0)
		self.smaa_fbo_:bind_output()
		
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

function render_pipeline:__update(dt)

    self.motion_scale_ = (1.0 / 60.0) / dt
end

function render_pipeline:final_pass(camera, input_tex)

    self.render_fbo_:clear()
    self.render_fbo_:attach(self.final_tex_, gl.COLOR_ATTACHMENT0)
    self.render_fbo_:bind_output()
    
    self.final_shader_:bind()
    
	root.Shader.get():uniform("u_InvViewProjMatrix", math.inverse(camera.view_proj_origin_matrix))
	root.Shader.get():uniform("u_PreViewProjMatrix", camera.prev_view_proj_origin_matrix)  
    root.Shader.get():uniform("u_EnableLensflare", self.enable_lensflare_ and 1 or 0)
	root.Shader.get():uniform("u_MotionScale", self.motion_scale_)
    
    gl.ActiveTexture(gl.TEXTURE0)
    root.Shader.get():sampler("s_Tex0", 0)
    self.gbuffer_depth_tex_:bind()
    
    gl.ActiveTexture(gl.TEXTURE1)
    root.Shader.get():sampler("s_Tex1", 1)
    input_tex:bind()
    
    if self.enable_lensflare_ then 
    
		local camx = vec3(math.column(camera.view_matrix, 0))
		local camz = vec3(math.column(camera.view_matrix, 2))	
		local rot = math.dot(camx, vec3(1, 0, 0)) * math.dot(camz, vec3(0, 0, 1))
		rot = rot * 4.0
		
		local rot_matrix = math.translate(mat4(), vec3(0.5, 0.5, 0.0))	
		rot_matrix = math.rotate(rot_matrix, rot, vec3(0.0, 0.0, 1.0))
		rot_matrix = math.translate(rot_matrix, vec3(-0.5, -0.5, 0.0))

        root.Shader.get():uniform("u_LensStarMatrix", rot_matrix)		
		
		gl.ActiveTexture(gl.TEXTURE2)
		root.Shader.get():sampler("s_Tex2", 2)
		self.lensflare_features_tex_:bind()
		
		gl.ActiveTexture(gl.TEXTURE3)
		root.Shader.get():sampler("s_Tex3", 3)
		self.lensflare_dirt_tex_:bind()
		
		gl.ActiveTexture(gl.TEXTURE4)
		root.Shader.get():sampler("s_Tex4", 4)
		self.lensflare_star_tex_:bind()
    end
    
    self.screen_quad_:draw()	
    root.Shader.pop_stack()

    root.FrameBuffer.unbind()
    return self.final_tex_
end

function render_pipeline:render(planet_camera, sector_camera, draw_func)

    if self.enable_shadow_ then
        self.scene_:make_shadow(sector_camera)
    end
    
    if self.enable_reflection_ then
        local sector = self.scene_.planet_.loaded_sectors_[1]
        sector.global_cubemap_:generate()
    end

    self.gbuffer_fbo_:clear()
    self.gbuffer_fbo_:attach(self.gbuffer_depth_tex_, gl.DEPTH_ATTACHMENT)
    self.gbuffer_fbo_:attach(self.gbuffer_diffuse_tex_, gl.COLOR_ATTACHMENT0)
    self.gbuffer_fbo_:attach(self.gbuffer_geometric_tex_, gl.COLOR_ATTACHMENT1)
    self.gbuffer_fbo_:bind_output()
    
    gl.ClearColor(root.Color.Transparent)
    gl.Clear(bit.bor(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT))
    draw_func(planet_camera, sector_camera)
    
    local occlusion = self:process_occlusion(planet_camera)
    local final_tex = self:create_hdr_render(planet_camera, sector_camera, occlusion)
    final_tex = self:process_bloom(final_tex)
    final_tex = self:process_lensflare(final_tex)    
    final_tex = self:perform_smaa(final_tex, self.render_smaa_tex_)
    return self:final_pass(planet_camera, final_tex)
end