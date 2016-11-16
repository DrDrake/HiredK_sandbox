--[[
- @file planet.h
- @brief
]]

class 'planet' (root.ScriptObject)

function planet:__init(scene, radius, max_level, enable_logz)
    root.ScriptObject.__init(self, "planet")
	self.scene_ = scene
	
	self.shader_ = FileSystem:search("shaders/gbuffer_planet.glsl", true)
    self.enable_logz_ = enable_logz
    self.radius_ = radius
    self.max_level_ = max_level
    self.faces_ = {}
    
    self.atmosphere_ = planet_atmosphere(self)
    self.atmosphere_:generate()
    
    self.clouds_ = planet_clouds(self)
    
    self.ocean_shader_ = FileSystem:search("shaders/gbuffer_ocean.glsl", true)
    self.ocean_ = planet_ocean(self)
    
    self.sectors_point_cloud_ = root.PointCloud(10)
    self.sectors_point_cloud_scale_ = (1.0 / (self.radius_ + 10.0)) * 10.0
    self.loaded_sectors_ = {}
    self.active_sectors_ = {}
    
	for i = 1, 6 do
    
		self.faces_[i] = root.Terrain(radius, max_level, i - 1)
        root.connect(self.faces_[i].on_page_in, function(tile)
        
            if tile.level == max_level - 2 then
                local deformed_center = tile.owner:get_face_matrix() * tile.owner:get_deformation():local_to_deformed(tile:get_center())
                self.active_sectors_ = {}
                
                local matches = self.sectors_point_cloud_:radius_search(deformed_center * self.sectors_point_cloud_scale_, 0.01)              
                for i = 1, table.getn(matches) do
                    self.active_sectors_[i] = self.loaded_sectors_[matches[i]]
                    self.active_sectors_[i]:page_in()
                end
            end
		end)
        
        self.faces_[i].spherical_deformation_ = root.SphericalDeformation(radius)
        self.faces_[i]:set_deformation(self.faces_[i].spherical_deformation_)
        
        self.faces_[i].producer_ = planet_producer(self, self.faces_[i], 321132)
        self.faces_[i]:add_producer(self.faces_[i].producer_)
        
        self:load_sector(self.faces_[i], vec3(0, 0, 0))
	end
    
    self.sectors_point_cloud_:build()
end

function planet:load_sector(face, position)

    local deformed_position = face:get_face_matrix() * face:get_deformation():local_to_deformed(position)
    local index = self.sectors_point_cloud_:add_point(deformed_position * self.sectors_point_cloud_scale_)
    self.loaded_sectors_[index] = planet_sector(self, face, deformed_position, index)
    return self.loaded_sectors_[index]
end

function planet:draw(planet_camera, sector_camera)

    self.shader_:bind()   
    root.Shader.get():uniform("u_EnableLogZ", self.enable_logz_ and 1 or 0)
    self.scene_:bind_global(planet_camera)
    
	for i = 1, 6 do
		self.faces_[i]:draw(planet_camera)
	end
    
    if self.ocean_.is_loaded_ then
    
        self.ocean_shader_:bind()
        self.scene_:bind_global(planet_camera)
        self.ocean_:draw(planet_camera)
        root.Shader.pop_stack()
    end
    
    for i = 1, table.getn(self.active_sectors_) do
        self.active_sectors_[i]:draw(planet_camera, sector_camera)
    end
    
    root.Shader.pop_stack()
end