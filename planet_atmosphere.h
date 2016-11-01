--[[
- @file planet_atmosphere.h
- @brief
]]

Rg = 6360.0
Rt = 6420.0
RL = 6421.0

TRANSMITTANCE_W = 256
TRANSMITTANCE_H = 64

SKY_W = 64
SKY_H = 16

RES_R = 32
RES_MU = 128
RES_MU_S = 32
RES_NU = 8

AVERAGE_GROUND_REFLECTANCE = 0.1

class 'planet_atmosphere' (root.ScriptObject)

function planet_atmosphere:__init(planet)
    root.ScriptObject.__init(self, "planet_atmosphere")
	self.planet_ = planet
	
	self.HR_ = 8.0
	self.betaR_ = vec3(5.8e-3, 1.35e-2, 3.31e-2)	
	
	self.HM_ = 1.2
	self.betaMSca_ = vec3(4e-3, 4e-3, 4e-3)
	self.betaMEx_ = vec3(4.44e-3, 4.44e-3, 4.44e-3)
	self.mieG_ = 0.8
	
	self.transmittance_tex_ = root.Texture.from_empty(TRANSMITTANCE_W, TRANSMITTANCE_H,
		{ iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
		
	self.irradiance_tex_ = root.Texture.from_empty(SKY_W, SKY_H,
		{ iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
	
	self.inscatter_tex_ = root.Texture.from_empty(RES_MU_S * RES_NU, RES_MU, RES_R,
		{ target = gl.TEXTURE_3D, iformat = gl.RGBA16F, format = gl.RGBA, type = gl.FLOAT, filter_mode = gl.LINEAR })
		
	self.glare_tex_ = root.Texture.from_image(FileSystem:search("sunglare.png", true),
		{ filter_mode = gl.LINEAR })

	self.copy_inscatter1_shader_ = FileSystem:search("shaders/atmosphere/copy_inscatter1.glsl", true)
	self.copy_inscatterN_shader_ = FileSystem:search("shaders/atmosphere/copy_inscatterN.glsl", true)
	self.copy_irradiance_shader_ = FileSystem:search("shaders/atmosphere/copy_irradiance.glsl", true)
	self.inscatter1_shader_ = FileSystem:search("shaders/atmosphere/inscatter1.glsl", true)
	self.inscatterN_shader_ = FileSystem:search("shaders/atmosphere/inscatterN.glsl", true)
	self.inscatterS_shader_ = FileSystem:search("shaders/atmosphere/inscatterS.glsl", true)	
	self.irradiance1_shader_ = FileSystem:search("shaders/atmosphere/irradiance1.glsl", true)
	self.irradianceN_shader_ = FileSystem:search("shaders/atmosphere/irradianceN.glsl", true)
	self.transmittance_shader_ = FileSystem:search("shaders/atmosphere/transmittance.glsl", true)
	self.screen_quad_ = root.Mesh.build_quad()
end

function planet_atmosphere:set_uniforms(scale)

	root.Shader.get():uniform("Rg", Rg * scale)
	root.Shader.get():uniform("Rt", Rt * scale)
	root.Shader.get():uniform("RL", RL * scale)
	
	root.Shader.get():sampler("TRANSMITTANCE_W", TRANSMITTANCE_W)
	root.Shader.get():sampler("TRANSMITTANCE_H", TRANSMITTANCE_H)
	
	root.Shader.get():sampler("SKY_W", SKY_W)
	root.Shader.get():sampler("SKY_H", SKY_H)
	
	root.Shader.get():sampler("RES_R", RES_R)
	root.Shader.get():sampler("RES_MU", RES_MU)
	root.Shader.get():sampler("RES_MU_S", RES_MU_S)
	root.Shader.get():sampler("RES_NU", RES_NU)
	
	root.Shader.get():uniform("AVERAGE_GROUND_REFLECTANCE", AVERAGE_GROUND_REFLECTANCE)
	
	root.Shader.get():uniform("HR", self.HR_ * scale)
	root.Shader.get():uniform("betaR", self.betaR_ / scale)	
	root.Shader.get():uniform("HM", self.HM_ * scale)
	root.Shader.get():uniform("betaMSca", self.betaMSca_ / scale)
	root.Shader.get():uniform("betaMEx", (self.betaMSca_ / scale) / 0.9)
	root.Shader.get():uniform("mieG", self.mieG_)
end

function planet_atmosphere:set_uniforms_layer(scale, layer)

	local r = layer / (RES_R - 1.0)
	r = math.sqrt(Rg * Rg + r * r * (Rt * Rt - Rg * Rg)) + ((layer == 0) and 0.01 or ((layer == RES_R - 1) and -0.001 or 0.0))
	
	local dmin = Rt - r
	local dmax = math.sqrt(r * r - Rg * Rg) + math.sqrt(Rt * Rt - Rg * Rg)
	local dminp = r - Rg
	local dmaxp = math.sqrt(r * r - Rg * Rg)
	
	root.Shader.get():uniform("r", r)
	root.Shader.get():uniform("dhdH", dmin, dmax, dminp, dmaxp)
	root.Shader.get():sampler("layer", layer)	
	self:set_uniforms(scale)
end

function planet_atmosphere:bind(scale)

	self:set_uniforms(scale)
	
	root.Shader.get():uniform("u_CameraPos", self.planet_.local_camera_.position)
	root.Shader.get():uniform("u_SunDir", self.planet_.scene_.sun_:get_direction())

	gl.ActiveTexture(gl.TEXTURE5)
	root.Shader.get():sampler("transmittanceSampler", 5)
	self.transmittance_tex_:bind()
	
	gl.ActiveTexture(gl.TEXTURE6)
	root.Shader.get():sampler("skyIrradianceSampler", 6)
	self.irradiance_tex_:bind()
	
	gl.ActiveTexture(gl.TEXTURE7)
	root.Shader.get():sampler("inscatterSampler", 7)
	self.inscatter_tex_:bind()
	
	gl.ActiveTexture(gl.TEXTURE8)
	root.Shader.get():sampler("glareSampler", 8)
	self.glare_tex_:bind()
end

function planet_atmosphere:generate()

	local fbo = root.FrameBuffer()

	local delta_e_tex = root.Texture.from_empty(SKY_W, SKY_H,
		{ iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
		
	local delta_sr_tex = root.Texture.from_empty(RES_MU_S * RES_NU, RES_MU, RES_R,
		{ target = gl.TEXTURE_3D, iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
		
	local delta_sm_tex = root.Texture.from_empty(RES_MU_S * RES_NU, RES_MU, RES_R,
		{ target = gl.TEXTURE_3D, iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
		
	local delta_j_tex = root.Texture.from_empty(RES_MU_S * RES_NU, RES_MU, RES_R,
		{ target = gl.TEXTURE_3D, iformat = gl.RGB16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR })
	
	local transmittance_pass = function()
	
		fbo:clear()
		fbo:attach(self.transmittance_tex_, gl.COLOR_ATTACHMENT0)
		fbo:bind_output()
		
		gl.Viewport(0, 0, TRANSMITTANCE_W, TRANSMITTANCE_H)	
		self.transmittance_shader_:bind()
		self:set_uniforms(1.0)
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
		
		root.FrameBuffer.unbind()
	end
	
	local irradiance_pass = function(delta_e_tex, transmittance_tex)
	
		fbo:clear()
		fbo:attach(delta_e_tex, gl.COLOR_ATTACHMENT0)
		fbo:bind_output()
		
		gl.Viewport(0, 0, SKY_W, SKY_H)	
		self.irradiance1_shader_:bind()
		self:set_uniforms(1.0)
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("transmittanceSampler", 0);
		transmittance_tex:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
	end
	
	local inscatter_pass = function(delta_sr_tex, delta_sm_tex, transmittance_tex)
	
		fbo:clear()
		fbo:attach(delta_sr_tex, gl.COLOR_ATTACHMENT0)
		fbo:attach(delta_sm_tex, gl.COLOR_ATTACHMENT1)
		fbo:bind_output()
		
		gl.Viewport(0, 0, RES_MU_S * RES_NU, RES_MU)	
		self.inscatter1_shader_:bind()
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("transmittanceSampler", 0);
		transmittance_tex:bind()
		
		for layer = 1, RES_R do
			self:set_uniforms_layer(1.0, layer - 1)
			self.screen_quad_:draw()
		end
		
		root.Shader.pop_stack()
	end
	
	local copy_irradiance_pass = function(irradiance_tex, k, delta_e_tex)
	
		fbo:clear()
		fbo:attach(irradiance_tex, gl.COLOR_ATTACHMENT0)
		fbo:bind_output()
		
		gl.Viewport(0, 0, SKY_W, SKY_H)	
		self.copy_irradiance_shader_:bind()
		root.Shader.get():uniform("k", k)
		self:set_uniforms(1.0)
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("deltaESampler", 0);
		delta_e_tex:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
		
		root.FrameBuffer.unbind()
	end
	
	local copy_inscatter_pass = function(delta_sr_tex, delta_sm_tex)
	
		fbo:clear()
		fbo:attach(self.inscatter_tex_, gl.COLOR_ATTACHMENT0)
		fbo:bind_output()
		
		gl.Viewport(0, 0, RES_MU_S * RES_NU, RES_MU)	
		self.copy_inscatter1_shader_:bind()
	
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("deltaSRSampler", 0);
		delta_sr_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("deltaSMSampler", 1);
		delta_sm_tex:bind()
		
		for layer = 1, RES_R do
			self:set_uniforms_layer(1.0, layer - 1)
			self.screen_quad_:draw()
		end
		
		root.Shader.pop_stack()
	end
	
	local compute_delta_j_pass = function(order, delta_j_tex, transmittance_tex, delta_e_tex, delta_sr_tex, delta_sm_tex)
	
		fbo:clear()
		fbo:attach(delta_j_tex, gl.COLOR_ATTACHMENT0)
		fbo:bind_output()
		
		gl.Viewport(0, 0, RES_MU_S * RES_NU, RES_MU)	
		self.inscatterS_shader_:bind()
		root.Shader.get():uniform("first", (order == 2) and 1.0 or 0.0)
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("transmittanceSampler", 0);
		transmittance_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("deltaESampler", 1);
		delta_e_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE2)
		root.Shader.get():sampler("deltaSRSampler", 2);
		delta_sr_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE3)
		root.Shader.get():sampler("deltaSMSampler", 3);
		delta_sm_tex:bind()
		
		for layer = 1, RES_R do
			self:set_uniforms_layer(1.0, layer - 1)
			self.screen_quad_:draw()
		end
		
		root.Shader.pop_stack()
	end
	
	local compute_delta_e_pass = function(order, delta_e_tex, transmittance_tex, delta_sr_tex, delta_sm_tex)
	
		fbo:clear()
		fbo:attach(delta_e_tex, gl.COLOR_ATTACHMENT0)
		fbo:bind_output()
		
		gl.Viewport(0, 0, SKY_W, SKY_H)	
		self.irradianceN_shader_:bind()
		root.Shader.get():uniform("first", (order == 2) and 1.0 or 0.0)
		self:set_uniforms(1.0)
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("transmittanceSampler", 0);
		transmittance_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("deltaSRSampler", 1);
		delta_sr_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE2)
		root.Shader.get():sampler("deltaSMSampler", 2);
		delta_sm_tex:bind()
		
		self.screen_quad_:draw()
		root.Shader.pop_stack()
	end
	
	local compute_delta_sr_pass = function(order, delta_sr_tex, transmittance_tex, delta_j_tex)
	
		fbo:clear()
		fbo:attach(delta_sr_tex, gl.COLOR_ATTACHMENT0)
		fbo:bind_output()
		
		gl.Viewport(0, 0, RES_MU_S * RES_NU, RES_MU)	
		self.inscatterN_shader_:bind()
		root.Shader.get():uniform("first", (order == 2) and 1.0 or 0.0)
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("transmittanceSampler", 0);
		transmittance_tex:bind()
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("deltaJSampler", 1);
		delta_j_tex:bind()
		
		for layer = 1, RES_R do
			self:set_uniforms_layer(1.0, layer - 1)
			self.screen_quad_:draw()
		end
		
		root.Shader.pop_stack()
	end
	
	local final_inscatter_pass = function(delta_sr_tex)
	
		fbo:clear()
		fbo:attach(self.inscatter_tex_, gl.COLOR_ATTACHMENT0)
		fbo:bind_output()
		
		gl.Viewport(0, 0, RES_MU_S * RES_NU, RES_MU)	
		self.copy_inscatterN_shader_:bind()
		
		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():sampler("deltaSSampler", 0);
		delta_sr_tex:bind()
		
		for layer = 1, RES_R do
			self:set_uniforms_layer(1.0, layer - 1)
			self.screen_quad_:draw()
		end
		
		root.Shader.pop_stack()
	end

	transmittance_pass()
	irradiance_pass(delta_e_tex, self.transmittance_tex_)
	inscatter_pass(delta_sr_tex, delta_sm_tex, self.transmittance_tex_)
	copy_irradiance_pass(self.irradiance_tex_, 0.0, delta_e_tex)
	copy_inscatter_pass(delta_sr_tex, delta_sm_tex)
	
	for order = 2, 4 do
		compute_delta_j_pass(order, delta_j_tex, self.transmittance_tex_, delta_e_tex, delta_sr_tex, delta_sm_tex)
		compute_delta_e_pass(order, delta_e_tex, self.transmittance_tex_, delta_sr_tex, delta_sm_tex)
		compute_delta_sr_pass(order, delta_sr_tex, self.transmittance_tex_, delta_j_tex)
		
		gl.Enable(gl.BLEND)
		gl.BlendFuncSeparate(gl.ONE, gl.ONE, gl.ONE, gl.ONE)
		gl.BlendEquationSeparate(gl.FUNC_ADD, gl.FUNC_ADD)
		
		copy_irradiance_pass(self.irradiance_tex_, 1.0, delta_e_tex)
		final_inscatter_pass(delta_sr_tex)
		
		gl.Disable(gl.BLEND)
	end
	
	root.FrameBuffer.unbind()
end