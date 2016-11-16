--[[
- @file scene.h
- @brief
]]

class 'scene' (root.ScriptObject)

function scene:__init()
    root.ScriptObject.__init(self, "scene")
	self.type = root.ScriptObject.Dynamic
    
    self.sun_ = sun(60.0, 20.0, 4096.0, 4)
    self.planet_ = nil
    
    self.depth_split_distance_ = 1500.0
    self.depth_split_clip_space_ = {}
end

function scene:is_planet_loaded()
    return self.planet_ ~= nil
end

function scene:load_planet(radius, max_level, enable_logz)
    self.planet_ = planet(self, radius, max_level, enable_logz)
    return self.planet_
end

function scene:__update(dt)
end

function scene:bind_global(camera)

    if self:is_planet_loaded() then
    
		local clip = camera.proj_matrix * vec4(0, 0, -self.depth_split_distance_, 1)
		self.depth_split_clip_space_ = (clip / clip.w).z
        
        root.Shader.get():uniform("u_DepthSplit", self.depth_split_clip_space_)
        root.Shader.get():uniform("u_EnableLogZ", self.planet_.enable_logz_ and 1 or 0)
        root.Shader.get():uniform("u_OceanLevel", 1.0)
        
        root.Shader.get():uniform("u_CameraClip", camera.clip)
        root.Shader.get():uniform("u_CameraPos", camera.position)
        
        root.Shader.get():uniform("u_SunDir", self.sun_:get_direction())
        self.planet_.atmosphere_:bind(1000.0)
        
        root.Shader.get():uniform_mat4_array("u_SunShadowMatrix", self.sun_.shadow_matrices_)	
        root.Shader.get():uniform("u_SunShadowSplits", vec4.from_table(self.sun_.shadow_splits_clip_space_))
        root.Shader.get():uniform("u_SunShadowResolution", 4096.0)
        root.Shader.get():uniform("u_SunDirection", self.sun_:get_direction())
        
        gl.ActiveTexture(gl.TEXTURE5)
        root.Shader.get():sampler("s_SunDepthTex", 5)
        self.sun_.shadow_depth_tex_array_:bind()
        
        if self.planet_.clouds_.is_loaded_ then
        
            local earth_radius = self.planet_.clouds_:calculate_planet_radius(self.planet_.clouds_.atmosphere_start_height_, self.planet_.clouds_.horizon_distance_)
            --print(6360000.0, earth_radius, 6360000.0 - earth_radius)
            root.Shader.get():uniform("u_EarthRadius", earth_radius)	
            root.Shader.get():uniform("u_StartHeight", self.planet_.clouds_.atmosphere_start_height_)	
            root.Shader.get():uniform("u_CoverageScale", 1.0 / self.planet_.clouds_:calculate_max_distance(earth_radius))
            root.Shader.get():uniform("u_CoverageOffset", self.planet_.clouds_.coverage_offset_)

        	gl.ActiveTexture(gl.TEXTURE6)
            root.Shader.get():sampler("s_Coverage", 6)
            self.planet_.clouds_.coverage_tex_:bind()
        end
    end
end

function scene:make_shadow(camera)

	gl.Enable(gl.DEPTH_TEST)

    if self:is_planet_loaded() then
        self.sun_:make_sun_shadow(camera, function(matrix)   
            for i = 1, table.getn(self.planet_.active_sectors_) do
            
                self.planet_.active_sectors_[i].gbuffer_shader_:bind()
                root.Shader.get():uniform("u_ViewProjMatrix", matrix)
                self.planet_.active_sectors_[i].model_:draw()
                root.Shader.pop_stack()
            end
        end)
    end
	
	gl.Disable(gl.DEPTH_TEST)
end

function scene:draw(planet_camera, sector_camera)

    gl.Enable(gl.DEPTH_TEST)
    gl.Enable(gl.CULL_FACE)

    if self:is_planet_loaded() then
        self.planet_:draw(planet_camera, sector_camera)
    end
    
    gl.Disable(gl.DEPTH_TEST)
    gl.Disable(gl.CULL_FACE)
end