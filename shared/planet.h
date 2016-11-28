--[[
- @file planet.h
- @brief
]]

class 'planet' (root.ScriptObject)

function planet:__init(scene, path)
    root.ScriptObject.__init(self, "planet")
	self.scene_ = scene
    self.path_ = path
end

function planet:build(radius, max_level, seed)

	self.shader_ = FileSystem:search("shaders/gbuffer_planet.glsl", true)
    self.radius_ = radius
    self.max_level_ = max_level
    self.seed_ = seed
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
        
            if tile.level == (max_level - 2) then
            
                local deformed_center = tile.owner:get_face_matrix() * tile.owner:get_deformation():local_to_deformed(tile:get_center())       
                local matches = self.sectors_point_cloud_:radius_search(deformed_center * self.sectors_point_cloud_scale_, 0.01)              
                for i = 1, table.getn(matches) do
                
                    if self.active_sectors_[i] == nil then
                        self.active_sectors_[i] = self.loaded_sectors_[matches[i]]
                        self.active_sectors_[i]:page_in()
                    end
                end
            end
		end)
        
        self.faces_[i].spherical_deformation_ = root.SphericalDeformation(radius)
        self.faces_[i]:set_deformation(self.faces_[i].spherical_deformation_)
        
        self.faces_[i].producer_ = planet_producer(self, self.faces_[i], seed)
        self.faces_[i]:add_producer(self.faces_[i].producer_)
	end
end

function planet:load_sector(face, position, path)

    local deformed_position = face:get_face_matrix() * face:get_deformation():local_to_deformed(position)
    local index = self.sectors_point_cloud_:add_point(deformed_position * self.sectors_point_cloud_scale_)
    self.loaded_sectors_[index] = planet_sector(self, face, position, deformed_position, index, path)
    return self.loaded_sectors_[index]
end

function planet:parse()

    local planet_config = json.parse(FileSystem:search(self.path_, true))
    self.name = planet_config:get_string("name")
    
    local seed = planet_config:get_number("seed")
    self:build(6360000.0, 14, seed)
    
    local sectors_array = planet_config:get_array("sectors")
    for i = 1, sectors_array.size do
    
        local sector_config = sectors_array:get(i - 1)
        local sector = self:load_sector(self.faces_[3], vec3(0, 0, 0), sector_config:get_string("path"))
        sector:parse()
    end
    
    self.sectors_point_cloud_:build()
end

function planet:write()

    local planet_config = json.Object()
    planet_config:set_string("name", self.name)
    planet_config:set_number("seed", self.seed_)  
    
    local sectors_array = json.Array()   
    for i = 1, table.getn(self.loaded_sectors_) do
    
        local sector_config = json.Object()
        sector_config:set_string("path", "sector.json")
        self.loaded_sectors_[i]:write()
        sectors_array:push(sector_config)
    end
    
    planet_config:set_array("sectors", sectors_array)    
    local data = json.Value(planet_config):serialize(true)
    FileSystem:write(self.path_, data)
end

function planet:draw(planet_camera, sector_camera)

    self.shader_:bind()
    root.Shader.get():uniform("u_SectorCameraPos", sector_camera.position)   
    self.scene_:bind_global(planet_camera)
    
	for i = 1, 6 do
		self.faces_[i]:draw(planet_camera)
	end
    
    if self.ocean_.is_loaded_ then
    
        self.ocean_shader_:bind()
        root.Shader.get():uniform("u_SectorCameraPos", sector_camera.position)   
        self.scene_:bind_global(planet_camera)
        self.ocean_:draw(planet_camera)
        root.Shader.pop_stack()
    end
    
    for i = 1, table.getn(self.active_sectors_) do
        self.active_sectors_[i]:draw(sector_camera)
    end
    
    root.Shader.pop_stack()
end