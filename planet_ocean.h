--[[
- @file planet_ocean.h
- @brief
]]

OCEAN_GRID_RESOLUTION = 8
N_SLOPE_VARIANCE = 10

GRID1_SIZE = 5488.0
GRID2_SIZE = 392.0
GRID3_SIZE = 28.0
GRID4_SIZE = 2.0

class 'planet_ocean' (root.ScriptObject)

function planet_ocean:__init(planet)
    root.ScriptObject.__init(self, "planet_ocean")
	self.type = root.ScriptObject.Dynamic
	
	self.planet_ = planet
	self.waves_ = root.WavesSpectrum()
	self.timer_ = 0.0
	
	self.zmin_ = 500.0
	self.draw_ocean_ = true
	self.offset_ = vec3()
	self.old_ltoo_ = mat4()
	self.test_ = 0
	
	self.variances_shader_ = FileSystem:search("shaders/ocean/variances.glsl", true)
	self.init_shader_ = FileSystem:search("shaders/ocean/init.glsl", true)
	self.fftx_shader_ = FileSystem:search("shaders/ocean/fftx.glsl", true)
	self.ffty_shader_ = FileSystem:search("shaders/ocean/ffty.glsl", true)
	
	local slope_variance_tex_settings_ = { target = gl.TEXTURE_3D, iformat = gl.R16F, format = gl.R, type = gl.FLOAT, filter_mode = gl.LINEAR }
	self.slope_variance_tex_ = root.Texture.from_empty(N_SLOPE_VARIANCE, N_SLOPE_VARIANCE, N_SLOPE_VARIANCE, slope_variance_tex_settings_)
	self.slope_variance_fbo_ = root.FrameBuffer()
	
	local fft_tex_settings = { target = gl.TEXTURE_2D_ARRAY, iformat = gl.RGBA16F, format = gl.RGB, type = gl.FLOAT, filter_mode = gl.LINEAR_MIPMAP_LINEAR, wrap_mode = gl.REPEAT, anisotropy = 16.0, mipmap = true }
	self.ffta_tex_ = root.Texture.from_empty(self.waves_.size, self.waves_.size, 5, fft_tex_settings)
	self.fftb_tex_ = root.Texture.from_empty(self.waves_.size, self.waves_.size, 5, fft_tex_settings)		
	self.fbo1_ = root.FrameBuffer()
	self.fbo2_ = root.FrameBuffer()	
	
	self.screen_grid_ = root.Mesh.build_grid(320, 180, true, false)
	self.screen_quad_ = root.Mesh.build_quad()
	self:compute_slope_variance_tex()
end

function planet_ocean:compute_slope_variance_tex()

	gl.Viewport(0, 0, N_SLOPE_VARIANCE, N_SLOPE_VARIANCE)
	self.variances_shader_:bind()
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("spectrum_1_2_Sampler", 0)
	self.waves_:bind(root.WavesSpectrum.SPECTRUM_12)
	
	gl.ActiveTexture(gl.TEXTURE1)
	root.Shader.get():sampler("spectrum_3_4_Sampler", 1)
	self.waves_:bind(root.WavesSpectrum.SPECTRUM_34)
	
	root.Shader.get():uniform("N_SLOPE_VARIANCE", N_SLOPE_VARIANCE)	
	root.Shader.get():sampler("FFT_SIZE", self.waves_.size)
	root.Shader.get():uniform("GRID_SIZES", GRID1_SIZE, GRID2_SIZE, GRID3_SIZE, GRID4_SIZE)
	root.Shader.get():uniform("slopeVarianceDelta", self.waves_.slope_variance_delta)
	
	for layer = 1, N_SLOPE_VARIANCE do
		self.slope_variance_fbo_:clear()
		self.slope_variance_fbo_:attach(self.slope_variance_tex_, gl.COLOR_ATTACHMENT0, 0, layer - 1)
		self.slope_variance_fbo_:bind_output()
		
		root.Shader.get():uniform("c", layer - 1)
		self.screen_quad_:draw()
	end
	
	root.FrameBuffer.unbind()
	root.Shader.pop_stack()
end

function planet_ocean:__update(dt)

	gl.Viewport(0, 0, self.waves_.size, self.waves_.size)
	self.timer_ = self.timer_ + dt
	
	self.fbo1_:clear()
	for i = 1, 5 do
		self.fbo1_:attach(self.ffta_tex_, gl.COLOR_ATTACHMENT0 + (i-1), 0, i-1)
	end	
	self.fbo1_:bind_output()
	
	self.init_shader_:bind()
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("spectrum_1_2_Sampler", 0)
	self.waves_:bind(root.WavesSpectrum.SPECTRUM_12)
	
	gl.ActiveTexture(gl.TEXTURE1)
	root.Shader.get():sampler("spectrum_3_4_Sampler", 1)
	self.waves_:bind(root.WavesSpectrum.SPECTRUM_34)
	
	root.Shader.get():uniform("t", self.timer_)
	
	root.Shader.get():uniform("FFT_SIZE", self.waves_.size)
	root.Shader.get():uniform("INVERSE_GRID_SIZES",
		2.0 * math.pi * self.waves_.size / GRID1_SIZE,
		2.0 * math.pi * self.waves_.size / GRID2_SIZE,
		2.0 * math.pi * self.waves_.size / GRID3_SIZE,
		2.0 * math.pi * self.waves_.size / GRID4_SIZE)
	
	self.screen_quad_:draw()
	root.Shader.pop_stack()
	
	self.fbo2_:clear()
	self.fbo2_:attach(self.ffta_tex_, gl.COLOR_ATTACHMENT0)
	self.fbo2_:attach(self.fftb_tex_, gl.COLOR_ATTACHMENT1)
	
	self.fftx_shader_:bind()
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("butterflySampler", 0)
	self.waves_:bind(root.WavesSpectrum.BUTTERFLY)
	
	for i = 1, 8 do
		root.Shader.get():uniform("pass", ((i - 1) + 0.5) / 8)
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("imgSampler", 1)
		
		if ((i - 1) % 2) == 0 then
			self.fbo2_:bind_output(gl.COLOR_ATTACHMENT1)
			self.ffta_tex_:bind()
		else
			self.fbo2_:bind_output(gl.COLOR_ATTACHMENT0)
			self.fftb_tex_:bind()
		end
		
		self.screen_quad_:draw()
	end
	
	root.Shader.pop_stack()
	
	self.ffty_shader_:bind()
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("butterflySampler", 0)
	self.waves_:bind(root.WavesSpectrum.BUTTERFLY)
	
	for i = 8 + 1, 2 * 8 do
		root.Shader.get():uniform("pass", ((i - 1) - 8 + 0.5) / 8)
		
		gl.ActiveTexture(gl.TEXTURE1)
		root.Shader.get():sampler("imgSampler", 1)
		
		if ((i - 1) % 2) == 0 then
			self.fbo2_:bind_output(gl.COLOR_ATTACHMENT1)
			self.ffta_tex_:bind()
		else
			self.fbo2_:bind_output(gl.COLOR_ATTACHMENT0)
			self.fftb_tex_:bind()
		end
		
		self.screen_quad_:draw()
	end
	
	self.ffta_tex_:generate_mipmap()
	
	root.FrameBuffer.unbind()
	root.Shader.pop_stack()
end

function planet_ocean:draw(camera)

	local ctol = math.inverse(camera.view_matrix)
	local cl = vec3(ctol * vec4(0, 0, 0, 1))	
	local radius = 6360000.0
	
	if math.length(cl) > radius + self.zmin_ then
		self.offset_ = vec3()
		self.old_ltoo_ = mat4()
		self.draw_ocean_ = false
		return
	end
	
	self.draw_ocean_ = true

	local uz = math.normalize(cl)
	local ux = math.normalize(math.cross(vec3(0, 0, 1), uz))
	local uy = math.cross(uz, ux)	
	local oo = uz * radius
	
	local ltoo = math.transpose(mat4.from_table({
		ux.x, ux.y, ux.z, -math.dot(ux, oo),
		uy.x, uy.y, uy.z, -math.dot(uy, oo),
		uz.x, uz.y, uz.z, -math.dot(uz, oo),
		0, 0, 0, 1
	}))
	
	local ctoo = ltoo * ctol
	
	if self.test_ > 1 then -- todo
		local delta = vec3(ltoo * (math.inverse(self.old_ltoo_) * vec4(0, 0, 0, 1)))
		self.offset_ = self.offset_ + delta
	end
	
	self.test_ = self.test_ + 1
	self.old_ltoo_ = ltoo
	
	local stoc = math.inverse(camera.proj_matrix)
	local oc = vec3(ctoo * vec4(0, 0, 0, 1))
	local h = oc.z
	
	local stoc_w = vec4(vec3(stoc * vec4(0, 0, 0, 1)), 0)
	local stoc_x = vec4(vec3(stoc * vec4(1, 0, 0, 0)), 0)
	local stoc_y = vec4(vec3(stoc * vec4(0, 1, 0, 0)), 0)
	
	local A0 = vec3(ctoo * stoc_w)
	local dA = vec3(ctoo * stoc_x)
	local B  = vec3(ctoo * stoc_y)
	
	local h1 = h * (h + 2.0 * radius)
	local h2 = (h + radius) * (h + radius)
	local alpha = math.dot(B, B) * h1 - B.z * B.z * h2
	local beta0 = (math.dot(A0, B) * h1 - B.z * A0.z * h2) / alpha
	local beta1 = (math.dot(dA, B) * h1 - B.z * dA.z * h2) / alpha
	local gamma0 = (math.dot(A0, A0) * h1 - A0.z * A0.z * h2) / alpha
	local gamma1 = (math.dot(A0, dA) * h1 - A0.z * dA.z * h2) / alpha
	local gamma2 = (math.dot(dA, dA) * h1 - dA.z * dA.z * h2) / alpha

	local horizon1 = vec3(-beta0, -beta1, 0)
	local horizon2 = vec3(beta0 * beta0 - gamma0, 2.0 * (beta0 * beta1 - gamma1), beta1 * beta1 - gamma2)
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("slopeVarianceSampler", 0)
	self.slope_variance_tex_:bind()
	
	gl.ActiveTexture(gl.TEXTURE1)
	root.Shader.get():sampler("fftWavesSampler", 1)
	self.ffta_tex_:bind()
	
	root.Shader.get():uniform("GRID_SIZES", GRID1_SIZE, GRID2_SIZE, GRID3_SIZE, GRID4_SIZE)	
	root.Shader.get():uniform("_Ocean_CameraToScreen", camera.proj_matrix)
	root.Shader.get():uniform("_Ocean_ScreenToCamera", stoc)
	root.Shader.get():uniform("_Ocean_CameraToOcean", ctoo)
	root.Shader.get():uniform("_Ocean_OceanToCamera", mat3(math.inverse(ctoo)))
	root.Shader.get():uniform("_Ocean_OceanToWorld", math.inverse(ltoo))
	root.Shader.get():uniform("_Ocean_Horizon1", horizon1)
	root.Shader.get():uniform("_Ocean_Horizon2", horizon2)
	root.Shader.get():uniform("_Ocean_CameraPos", vec3(-self.offset_.x, -self.offset_.y, oc.z))
	root.Shader.get():uniform("_Ocean_ScreenGridSize", OCEAN_GRID_RESOLUTION / 1280, OCEAN_GRID_RESOLUTION / 720)
	root.Shader.get():uniform("_Ocean_Radius", radius)	
	self.planet_.atmosphere_:bind(1000.0)
	
	self.screen_grid_:draw()
end