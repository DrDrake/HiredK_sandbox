--[[
- @file post_fx.h
- @brief
]]

class 'post_fx' (root.ScriptObject)

function post_fx:__init()
    root.ScriptObject.__init(self, "post_fx")
	
	self.sky_shader_ = FileSystem:search("shaders/sky.glsl", true)
	self.light_deferred_shader_ = FileSystem:search("shaders/light_deferred.glsl", true)
	self.scalebias_shader_ = FileSystem:search("shaders/scalebias.glsl", true)
	self.bilateral_shader_ = FileSystem:search("shaders/bilateral.glsl", true)
	self.gauss1d_shader_ = FileSystem:search("shaders/gauss1d.glsl", true)
	self.post_fx_shader_ = FileSystem:search("shaders/post_fx.glsl", true)
	self.screen_quad_ = root.Mesh.build_quad()
	
	-- occlusion
	self.hbao_shader_ = FileSystem:search("shaders/hbao.glsl", true)
	self.hbao_random_tex_ = root.Texture.from_random(4, 4)
	
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

function post_fx:rebuild(w, h)
	
	self.render_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.render_fbo_ = root.FrameBuffer()
	
	-- occlusion
	self.occlusion_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RG16F, format = gl.RG, type = gl.FLOAT })
	self.occlusion_blur_tex_ = root.Texture.from_empty(w, h, { iformat = gl.RG16F, format = gl.RG, type = gl.FLOAT })
	self.bilateral_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
	self.hbao_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
	
	-- bloom
	self.bloom_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.bloom_blur_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
	
	-- lensflare
	self.lensflare_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGBA16F, format = gl.RGBA, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.lensflare_features_tex_ = root.Texture.from_empty(w / 4, h / 4, { iformat = gl.RGBA16F, format = gl.RGBA, type = gl.FLOAT, filter_mode = gl.LINEAR })
	
	-- smaa
	self.smaa_edge_tex_ = root.Texture.from_empty(w, h, { type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.smaa_blend_tex_ = root.Texture.from_empty(w, h, { type = gl.FLOAT, filter_mode = gl.LINEAR })	
	self.smaa_final_tex_ = root.Texture.from_empty(w, h, { type = gl.FLOAT, filter_mode = gl.LINEAR })	
	self.smaa_edge_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
	self.smaa_blend_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))	
	self.smaa_final_shader_:override_define("RESOLUTION", string.format("vec2(%d, %d)", w, h))
end

function post_fx:perform_blur(shader, input_tex, blur_tex)
	
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

function post_fx:process_occlusion(camera, depth_tex)

	local generate_occlusion = function(depth_tex)
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.occlusion_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()	
		
		self.hbao_shader_:bind()
	
		local focal_len = vec2()
		focal_len.x = 1.0 / math.tan(camera.fov * 0.5) * (depth_tex.h / depth_tex.w)
		focal_len.y = 1.0 / math.tan(camera.fov * 0.5)
		
		local lin_mad = vec2()
		lin_mad.x = (camera.clip.x - camera.clip.y) / (2.0 * camera.clip.x * camera.clip.y)
		lin_mad.y = (camera.clip.x + camera.clip.y) / (2.0 * camera.clip.x * camera.clip.y)
		
		self.hbao_shader_:uniform("u_UVToViewA", -2.0 * (1.0 / focal_len.x), -2.0 * (1.0 / focal_len.y))
		self.hbao_shader_:uniform("u_UVToViewB", 1.0 / focal_len.x, 1.0 / focal_len.y)
		self.hbao_shader_:uniform("u_FocalLen", focal_len)
		self.hbao_shader_:uniform("u_LinMAD", lin_mad)
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		depth_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("s_Tex1", 1)
		self.hbao_random_tex_:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
	end
	
	generate_occlusion(depth_tex)
	self:perform_blur(self.bilateral_shader_, self.occlusion_tex_, self.occlusion_blur_tex_)
	return self.occlusion_tex_
end

function post_fx:process_hdr(dt, camera, scene, depth_tex, diffuse_tex, geometric_tex, occlusion)

	local draw_sky = function(scene)
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.render_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		gl.ClearColor(root.Color.Black)
		gl.Clear(gl.COLOR_BUFFER_BIT)
		
		self.sky_shader_:bind()
		root.Shader.get():uniform("u_InvProjMatrix", math.inverse(scene.planet_.local_camera_.proj_matrix))
		root.Shader.get():uniform("u_InvViewMatrix", math.inverse(scene.planet_.local_camera_.view_matrix))
		scene.planet_.atmosphere_:bind(1000.0)
		
		self.screen_quad_:draw()	
		root.Shader.pop_stack()
	end
	
	local deferred_light_pass = function(camera, scene, depth_tex, diffuse_tex, geometric_tex, occlusion)
	
		self.light_deferred_shader_:bind()	
		gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
		gl.Enable(gl.BLEND)
		
		root.Shader.get():uniform("u_InvViewProjMatrix", math.inverse(camera.view_proj_matrix))
		root.Shader.get():uniform("u_InvViewProjOriginMatrix", math.inverse(camera.view_proj_origin_matrix))
		scene.planet_.atmosphere_:bind(1000.0)
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		depth_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("s_Tex1", 1)
		diffuse_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE2)
		root.Shader.get():sampler("s_Tex2", 2)
		geometric_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE3)
		root.Shader.get():sampler("s_Tex3", 3)
		occlusion:bind()
		
		-- ss shadow
		root.Shader.get():uniform_mat4_array("u_SunShadowMatrix", scene.sun_.shadow_matrices_)
		root.Shader.get():uniform("u_SunShadowSplits", vec4.from_table(scene.sun_.shadow_splits_clip_space_))
		
		gl.ActiveTexture(gl.TEXTURE4)
		root.Shader.get():sampler("s_SunDepthTex", 4)
		scene.sun_.shadow_depth_tex_array_:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
		gl.Disable(gl.BLEND)
	end

	draw_sky(scene)
	deferred_light_pass(camera, scene, depth_tex, diffuse_tex, geometric_tex, occlusion)
	return self.render_tex_
end

function post_fx:process_bloom(input_tex)

	local generate_bloom = function(input_tex)
	
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
	
	local apply_bloom = function(input_tex, bloom_tex)
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(input_tex, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		gl.PushAttrib(gl.VIEWPORT_BIT)
		gl.Viewport(0, 0, input_tex.w, input_tex.h)	
		self.scalebias_shader_:bind()
		
		gl.Enable(gl.BLEND)
		gl.BlendFunc(gl.ONE, gl.ONE)
		
		root.Shader.get():uniform("u_Scale", vec4(1.0))
		root.Shader.get():uniform("u_Bias", vec4(0.0))
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		bloom_tex:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
		gl.Disable(gl.BLEND)
		gl.PopAttrib()
	end

	generate_bloom(input_tex)
	self:perform_blur(self.gauss1d_shader_, self.bloom_tex_, self.bloom_blur_tex_)
	apply_bloom(input_tex, self.bloom_tex_)
	return input_tex
end

function post_fx:process_lensflare(input_tex)

	local generate_lensflare = function(input_tex)
	
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
	
	local generate_lensflare_features = function(lensflare_tex)
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.lensflare_features_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		gl.PushAttrib(gl.VIEWPORT_BIT)
		gl.Viewport(0, 0, self.lensflare_features_tex_.w, self.lensflare_features_tex_.h)	
		self.lensflare_shader_:bind()
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		lensflare_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("s_Tex1", 1)
		self.lensflare_color_tex_:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
		gl.PopAttrib()
	end

	generate_lensflare(input_tex)
	generate_lensflare_features(self.lensflare_tex_)
	self:perform_blur(self.gauss1d_shader_, self.lensflare_features_tex_, self.lensflare_tex_)
	return input_tex
end

function post_fx:process_smaa(input_tex)

	local edge_pass = function(input_tex)
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.smaa_edge_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		gl.Clear(gl.COLOR_BUFFER_BIT)
		
		self.smaa_edge_shader_:bind()	
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		input_tex:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
	end
	
	local blend_pass = function(edge_tex, area_tex, search_tex)
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.smaa_blend_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		self.smaa_blend_shader_:bind()
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		edge_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("s_Tex1", 1)
		area_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE2)
		root.Shader.get():sampler("s_Tex2", 2)
		search_tex:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
	end
	
	local final_pass = function(input_tex, blend_tex)
	
		self.render_fbo_:clear()
		self.render_fbo_:attach(self.smaa_final_tex_, gl.COLOR_ATTACHMENT0)
		self.render_fbo_:bind_output()
		
		self.smaa_final_shader_:bind()
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("s_Tex0", 0)
		input_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("s_Tex1", 1)
		blend_tex:bind()
		
		self.screen_quad_:draw()	
		root.Shader.pop_stack()
	end
	
	edge_pass(input_tex)
	blend_pass(self.smaa_edge_tex_, self.smaa_area_tex_, self.smaa_search_tex_)
	final_pass(input_tex, self.smaa_blend_tex_)
	return self.smaa_final_tex_
end

function post_fx:process(dt, camera, scene, depth_tex, diffuse_tex, geometric_tex)

	local occlusion = self:process_occlusion(camera, depth_tex)
	local final_tex = self:process_hdr(dt, camera, scene, depth_tex, diffuse_tex, geometric_tex, occlusion)
	final_tex = self:process_bloom(final_tex)
	final_tex = self:process_lensflare(final_tex)
	final_tex = self:process_smaa(final_tex)
	root.FrameBuffer.unbind()
	
	self.post_fx_shader_:bind()
	
	root.Shader.get():uniform("u_InvViewProjMatrix", math.inverse(camera.view_proj_origin_matrix))
	root.Shader.get():uniform("u_PreViewProjMatrix", camera.prev_view_proj_origin_matrix_)
	root.Shader.get():uniform("u_MotionScale", (1.0 / 60.0) / dt)	
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("s_Tex0", 0)
	depth_tex:bind()

	gl.ActiveTexture(gl.TEXTURE1)
	root.Shader.get():sampler("s_Tex1", 1)
	final_tex:bind()
	
	if true then -- lensflare enabled
	
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
end