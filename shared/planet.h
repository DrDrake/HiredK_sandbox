--[[
- @file planet.h
- @brief
]]

class 'planet' (root.ScriptObject)

function planet:__init()
    root.ScriptObject.__init(self, "planet")
    self.type = root.ScriptObject.Dynamic
    
    self.planet_camera_ = root.Camera()
    self.shader_ = nil
    self.radius_ = 0.0
    self.max_level_ = 0
    self.seed_ = 0
    self.objects_ = {}
    self.atmosphere_ = nil
    self.clouds_ = nil
    self.ocean_ = nil
    self.sun_ = nil
    self.global_probe_ = nil
    self.sectors_point_cloud_ = nil
    self.sectors_point_cloud_scale_ = 1.0
    self.loaded_sectors_ = {}
    self.active_sectors_ = {}
    self.faces_ = {}
    self.albedo_detail_tex_array_ = nil -- albedo.rgb / roughtness.a
    self.normal_detail_tex_array_ = nil -- normal.rgb / height.a
	self.deferred_dist_ = 15000.0
	self.deferred_dist_clip_space_ = 0
	self.details_dist_ = 500.0
	self.details_dist_clip_space_ = 0
	self.parallax_dist_ = 50.0
	self.parallax_dist_clip_space_ = 0
    self.loaded_ = false
end

function planet:load(radius, max_level, seed)

    self.shader_ = FileSystem:search("shaders/gbuffer_planet.glsl", true)
    self.radius_ = radius
    self.max_level_ = max_level
    self.seed_ = seed
    
    self.atmosphere_ = atmosphere()
    self.atmosphere_:generate()   
    self.clouds_ = volumetric_clouds()
    self.ocean_ = ocean()
    
    self.sun_ = directional_light(self, 4, 4096)
    table.insert(self.objects_, self.sun_)   
    self.global_probe_ = envprobe(self, 128)
    table.insert(self.objects_, self.global_probe_)
    
    self.sectors_point_cloud_ = root.PointCloud(10)
    self.sectors_point_cloud_scale_ = (1.0 / (self.radius_ + 10.0)) * 10.0
    self.loaded_sectors_ = {}
    self.active_sectors_ = {}

    for i = 1, 6 do
        self.faces_[i] = root.Terrain(radius, max_level, i - 1)
        
        -- apply spherical deformation
        self.faces_[i].spherical_deformation_ = root.SphericalDeformation(radius)
        self.faces_[i]:set_deformation(self.faces_[i].spherical_deformation_)
        
        -- apply default tile producer
        self.faces_[i].producer_ = planet_producer(self, self.faces_[i], seed)
        self.faces_[i]:add_producer(self.faces_[i].producer_)
        
        root.connect(self.faces_[i].on_page_in, function(tile)
            if tile.level == max_level - 2 then
            
                local deformed = tile.owner:get_face_matrix() * tile.owner:get_deformation():local_to_deformed(tile:get_center())     
                local matches = self.sectors_point_cloud_:radius_search(deformed * self.sectors_point_cloud_scale_, 0.01)
                for i = 1, table.getn(matches) do
                
                    if self.active_sectors_[i] == nil then
                        self.active_sectors_[i] = self.loaded_sectors_[matches[i]]
                        self.active_sectors_[i]:page_in()
                    end
                end
            end
        end)
    end
    
    self.albedo_detail_tex_array_ = root.Texture.from_images(
        { FileSystem:search("images/sand.png", true), FileSystem:search("images/grass.png", true), FileSystem:search("images/rock.png", true) },
        { wrap_mode = gl.REPEAT, filter_mode = gl.LINEAR_MIPMAP_LINEAR, anisotropy = 16, mipmap = true }
    )
    
    self.normal_detail_tex_array_ = root.Texture.from_images(
        { FileSystem:search("images/sand_n.png", true), FileSystem:search("images/grass_n.png", true), FileSystem:search("images/rock_n.png", true) },
        { wrap_mode = gl.REPEAT, filter_mode = gl.LINEAR_MIPMAP_LINEAR, anisotropy = 16, mipmap = true }
    )
    
    local sector = self:load_sector(self.faces_[3], vec3(0, 0, 0))
	
	local obj1 = box_object(self, vec3(2.5), 1.0)
	obj1.transform_.position = vec3(97.623315481981, 38, 658.82348331446)
    table.insert(sector.objects_, obj1)
	
	local obj2 = model_object(self, "scene.glb", 0.0)
	obj2.transform_.position = vec3(141.75627243072, 25.0, 682.65424162731)
	table.insert(sector.objects_, obj2)
	
	local obj3 = tree_object(self, "tree.glb")
	obj3.transform_.position = vec3(128.32568209607, 21.5, 740.26017522014)
	obj3.transform_.scale = vec3(8)
	table.insert(sector.objects_, obj3)
	
	local plight1 = point_light(self, 50.0)
	plight1.transform_.position = vec3(146.38941847228, 34, 639.25798576049)
	plight1.color_ = root.Color.Red
	table.insert(sector.objects_, plight1)
	
	local plight2 = point_light(self, 50.0)
	plight2.transform_.position = vec3(148.88699658612, 34, 639.25798576049)
	plight2.color_ = root.Color.Green
	table.insert(sector.objects_, plight2)
	
	local plight3 = point_light(self, 50.0)
	plight3.transform_.position = vec3(136.77103958925, 34, 705.76084004379)
	plight3.color_ = root.Color.Blue
	table.insert(sector.objects_, plight3)
	
	self.global_probe_:add_local_probe(vec3(158.5, 32, 706), vec3(44.5, 15, 23), 32, { obj2, obj3 })
	
    self.sectors_point_cloud_:build()   
    self.loaded_ = true
end

function planet:load_sector(face, position)

    local deformed = face:get_face_matrix() * face:get_deformation():local_to_deformed(position)
    local index = self.sectors_point_cloud_:add_point(deformed * self.sectors_point_cloud_scale_)   
    self.loaded_sectors_[index] = planet_sector(self, face, deformed)
    return self.loaded_sectors_[index]
end

function planet:__update(dt)

	if sf.Keyboard.is_key_pressed(sf.Keyboard.Up) then
		self.sun_.coord_.x = self.sun_.coord_.x - 15.0 * dt
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.Down) then
		self.sun_.coord_.x = self.sun_.coord_.x + 15.0 * dt
	end
	
	if sf.Keyboard.is_key_pressed(sf.Keyboard.Left) then
		self.sun_.coord_.y = self.sun_.coord_.y - 15.0 * dt
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.Right) then
		self.sun_.coord_.y = self.sun_.coord_.y + 15.0 * dt
	end
end

function planet:draw(camera)

	if sf.Keyboard.is_key_pressed(sf.Keyboard.P) then
		print(camera.position.x, camera.position.y, camera.position.z)
		print(self.sun_.coord_.x, self.sun_.coord_.y)
	end

    if self.loaded_ then
    
        self.planet_camera_:copy(camera)
        self.planet_camera_.position.y = self.planet_camera_.position.y + 6360000.0
        self.planet_camera_:refresh()
		
		local clip = self.planet_camera_.proj_matrix * vec4(0, 0, -self.deferred_dist_, 1)
		self.deferred_dist_clip_space_ = (clip / clip.w).z	
		clip = self.planet_camera_.proj_matrix * vec4(0, 0, -self.details_dist_, 1)
		self.details_dist_clip_space_ = (clip / clip.w).z		
		clip = self.planet_camera_.proj_matrix * vec4(0, 0, -self.parallax_dist_, 1)
		self.parallax_dist_clip_space_ = (clip / clip.w).z
		
        self.shader_:bind()
        root.Shader.get():uniform("u_WorldCamPosition", self.planet_camera_.position)
		root.Shader.get():uniform("u_LocalCamPosition", camera.position)
		root.Shader.get():uniform("u_SunDirection", self.sun_:get_direction())
		root.Shader.get():uniform("u_DeferredDist", self.deferred_dist_clip_space_)
		root.Shader.get():uniform("u_DetailsDist", self.details_dist_clip_space_)
		root.Shader.get():uniform("u_ParallaxDist", self.parallax_dist_clip_space_)
		self.atmosphere_:bind(1000)
		self.clouds_:bind_coverage()
                      
        gl.ActiveTexture(gl.TEXTURE3)
        root.Shader.get():sampler("s_AlbedoDetailTexArray", 3)
        self.albedo_detail_tex_array_:bind()
        
        gl.ActiveTexture(gl.TEXTURE4)
        root.Shader.get():sampler("s_NormalDetailTexArray", 4)
        self.normal_detail_tex_array_:bind()
		
        for i = 1, 6 do
            self.faces_[i]:draw(self.planet_camera_)
        end
		
        root.Shader.pop_stack()
        
        for i = 1, table.getn(self.active_sectors_) do
            self.active_sectors_[i]:__draw(camera)
        end
        
        self.ocean_:draw(self.planet_camera_, self.radius_)
    end
end