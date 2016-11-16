--[[
- @file planet_clouds.h
- @brief
]]

class 'planet_clouds' (root.ScriptObject)

function planet_clouds:__init(planet)
    root.ScriptObject.__init(self, "planet_clouds")
    self.type = root.ScriptObject.Dynamic
	self.planet_ = planet
    self.is_loaded_ = false
end

function planet_clouds:load()

    if self.is_loaded_ == false then
    
        self.max_iterations_ = 80	
        self.random_vectors_ = {}
        
        for i = 1, 6 do
            self.random_vectors_[i] = math.ball_rand(1.0)
        end
            
        self.coverage_tex_ = root.Texture.from_image(FileSystem:search("coverage.png", true),
            { filter_mode = gl.LINEAR, wrap_mode = gl.REPEAT } )
            
        self.curl_tex_ = root.Texture.from_image(FileSystem:search("curl.png", true),
            { filter_mode = gl.LINEAR, wrap_mode = gl.REPEAT } )
            
        self.perlin3d_tex_ = root.Texture.from_raw(128, 128, 128, FileSystem:search("perlin3d.raw", true),
            { target = gl.TEXTURE_3D, filter_mode = gl.LINEAR, wrap_mode = gl.REPEAT } )
            
        self.detail_tex_ = root.Texture.from_raw(32, 32, 32, FileSystem:search("detail.raw", true),
            { target = gl.TEXTURE_3D, iformat = gl.RGB8, format = gl.RGB, filter_mode = gl.LINEAR, wrap_mode = gl.REPEAT } )
        
        -- coverage
        self.coverage_offset_ = vec2()
        self.horizon_coverage_start_ = 0.3
        self.horizon_coverage_end_ = 0.45
        
        -- lighting
        self.cloud_base_color_ = vec3(0.518, 0.666, 0.816)
        self.cloud_top_color_ = vec3(1.000, 1.000, 1.000)
        self.sun_scalar_ = 1.0
        self.ambient_scalar_ = 1.0
        self.sunray_length_ = 0.08
        self.cone_radius_ = 0.08
        self.density_ = 1.0
        self.forward_scattering_G_ = 0.8
        self.backward_scattering_G_ = -0.5
        self.dark_outline_scalar_ = 1.0
        
        -- animation
        self.animation_scale_ = 1.0
        self.coverage_offset_per_frame_ = vec2(0.0004, 0.001)
        self.base_offset_per_frame_ = vec3(0.0, 0.0, 0.01)
        self.detail_offset_per_frame_ = vec3()
        
        -- modeling (base)	
        self.base_scale_ = 1.0
        self.base_offset_ = vec3()
        self.cloud_gradient1_ = vec4(0.011, 0.098, 0.126, 0.225)
        self.cloud_gradient2_ = vec4(0.000, 0.096, 0.311, 0.506)
        self.cloud_gradient3_ = vec4(0.000, 0.087, 0.749, 1.000)
        self.sample_scalar_ = 1.0
        self.sample_threshold_ = 0.05
        self.cloud_bottom_fade_ = 0.0
        
        -- modeling (detail)
        self.detail_scale_ = 8.0
        self.detail_offset_ = vec3()
        self.erosion_edge_size_ = 0.5
        self.cloud_distortion_ = 0.45
        self.cloud_distortion_scale_ = 0.5
        
        -- optimization
        self.LOD_distance_ = 0.3
        self.horizon_level_ = 0.0
        self.horizon_fade_ = 0.25
        self.horizon_fade_start_alpha_ = 0.9
        
        -- atmosphere
        self.horizon_distance_ = 35000.0
        self.atmosphere_start_height_ = 1500.0
        self.atmosphere_end_height_ = 4000.0
        self.camera_position_scaler_ = vec3(1.0)
        
        self.is_loaded_ = true
    end
end

function planet_clouds:calculate_horizon_distance(inner_radius, outer_radius)
	return math.sqrt((outer_radius * outer_radius) - (inner_radius * inner_radius))
end

function planet_clouds:calculate_max_distance(radius)
	return self:calculate_horizon_distance(radius, radius + self.atmosphere_end_height_)
end

function planet_clouds:calculate_max_ray_distance(radius)
	local inner_distance = self:calculate_horizon_distance(radius, radius + self.atmosphere_start_height_)
	local outer_distance = self:calculate_horizon_distance(radius, radius + self.atmosphere_end_height_)
	return outer_distance - inner_distance
end

function planet_clouds:calculate_planet_radius(atmosphere_height, horizon_distance)
	local radius = atmosphere_height * atmosphere_height + horizon_distance * horizon_distance
	radius = radius / (2.0 * atmosphere_height)
	return radius - atmosphere_height
end

function planet_clouds:set_uniforms()

	root.Shader.get():uniform("_CloudBottomFade", self.cloud_bottom_fade_)
	root.Shader.get():uniform("_MaxIterations", self.max_iterations_)
	root.Shader.get():uniform("_SampleScalar", self.sample_scalar_)
	root.Shader.get():uniform("_SampleThreshold", self.sample_threshold_)
	root.Shader.get():uniform("_LODDistance", self.LOD_distance_)
	root.Shader.get():uniform("_RayMinimumY", self.horizon_level_)
	root.Shader.get():uniform("_ErosionEdgeSize", self.erosion_edge_size_)
	root.Shader.get():uniform("_CloudDistortion", self.cloud_distortion_)
	root.Shader.get():uniform("_CloudDistortionScale", self.cloud_distortion_scale_)
	root.Shader.get():uniform("_LightScalar", self.sun_scalar_)
	root.Shader.get():uniform("_AmbientScalar", self.ambient_scalar_)
	root.Shader.get():uniform("_CloudBaseColor", self.cloud_base_color_)
	root.Shader.get():uniform("_CloudTopColor", self.cloud_top_color_)
	root.Shader.get():uniform("_HorizonCoverageStart", self.horizon_coverage_start_)
	root.Shader.get():uniform("_HorizonCoverageEnd", self.horizon_coverage_end_)
	root.Shader.get():uniform("_Density", self.density_)
	
	root.Shader.get():uniform("_HorizonFadeScalar", self.horizon_fade_)
	root.Shader.get():uniform("_HorizonFadeStartAlpha", self.horizon_fade_start_alpha_)
	root.Shader.get():uniform("_OneMinusHorizonFadeStartAlpha", 1.0 - self.horizon_fade_start_alpha_)
	
	root.Shader.get():uniform("_BaseScale", 1.0 / self.atmosphere_end_height_ * self.base_scale_)
	root.Shader.get():uniform("_BaseOffset", self.base_offset_)
	
	root.Shader.get():uniform("_DetailScale", self.detail_scale_)
	root.Shader.get():uniform("_DetailOffset", self.detail_offset_)
	
	root.Shader.get():uniform("_CloudHeightGradient1", self.cloud_gradient1_)
	root.Shader.get():uniform("_CloudHeightGradient2", self.cloud_gradient2_)
	root.Shader.get():uniform("_CloudHeightGradient3", self.cloud_gradient3_)
	
	root.Shader.get():uniform("_ForwardScatteringG", self.forward_scattering_G_)
	root.Shader.get():uniform("_BackwardScatteringG", self.backward_scattering_G_)
	root.Shader.get():uniform("_DarkOutlineScalar", self.dark_outline_scalar_)
	
	local atmosphere_thickness = self.atmosphere_end_height_ - self.atmosphere_start_height_
	root.Shader.get():uniform("_SunRayLength", self.sunray_length_ * atmosphere_thickness)
	root.Shader.get():uniform("_ConeRadius", self.cone_radius_ * atmosphere_thickness)
	root.Shader.get():uniform("_RayStepLength", atmosphere_thickness / math.floor(self.max_iterations_ * 0.5))
	
	local earth_radius = self:calculate_planet_radius(self.atmosphere_start_height_, self.horizon_distance_)
    
	root.Shader.get():uniform("_CoverageScale", 1.0 / self:calculate_max_distance(earth_radius))
	root.Shader.get():uniform("_CoverageOffset", self.coverage_offset_)
	root.Shader.get():uniform("_MaxRayDistance", self:calculate_max_ray_distance(earth_radius))
	
	root.Shader.get():uniform("_Random0", self.random_vectors_[1])
	root.Shader.get():uniform("_Random1", self.random_vectors_[2])
	root.Shader.get():uniform("_Random2", self.random_vectors_[3])
	root.Shader.get():uniform("_Random3", self.random_vectors_[4])
	root.Shader.get():uniform("_Random4", self.random_vectors_[5])
	root.Shader.get():uniform("_Random5", self.random_vectors_[6])
	
	root.Shader.get():uniform("_EarthRadius", earth_radius)
	root.Shader.get():uniform("_StartHeight", self.atmosphere_start_height_)
	root.Shader.get():uniform("_EndHeight", self.atmosphere_end_height_)
	root.Shader.get():uniform("_AtmosphereThickness", self.atmosphere_end_height_ - self.atmosphere_start_height_)
	root.Shader.get():uniform("_MaxDistance", self:calculate_max_distance(earth_radius))
	
	root.Shader.get():uniform("_LightDirection", self.planet_.scene_.sun_:get_direction())
	root.Shader.get():uniform("_LightColor", vec3(1.0))
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("_Coverage", 0)
	self.coverage_tex_:bind()
	
	gl.ActiveTexture(gl.TEXTURE1)
	root.Shader.get():sampler("_Curl2D", 1)
	self.curl_tex_:bind()
	
	gl.ActiveTexture(gl.TEXTURE2)
	root.Shader.get():sampler("_Perlin3D", 2)
	self.perlin3d_tex_:bind()
	
	gl.ActiveTexture(gl.TEXTURE3)
	root.Shader.get():sampler("_Detail3D", 3)
	self.detail_tex_:bind()
end

function planet_clouds:__update(dt)

    if self.is_loaded_ then
        self.coverage_offset_ = self.coverage_offset_ + (self.coverage_offset_per_frame_ * dt)
        self.base_offset_ = self.base_offset_ + (self.base_offset_per_frame_ * dt)
    end
end