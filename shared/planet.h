--[[
- @file planet.h
- @brief
]]

class 'planet' (root.ScriptObject)

function planet:__init(path)
    root.ScriptObject.__init(self, "planet")
	self.type = root.ScriptObject.Dynamic
	self.path_ = path
	
	self.shader_ = FileSystem:search("shaders/gbuffer_planet.glsl", true)
	self.logz_shader_ = FileSystem:search("shaders/gbuffer_planet_logz.glsl", true)
	self.enable_logz_ = false
	self.spherical_ = true
	self.radius_ = 0
	self.max_level_ = 0
	self.seed_ = 0
	
	self.atmosphere_ = nil
	self.clouds_ = nil
	self.ocean_ = nil
	self.sun_ = nil
	self.global_envprobe_ = nil
	self.objects_ = {}
	
	self.sectors_cloud_ = root.PointCloud(10)
	self.sectors_cloud_scale_ = 1.0
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
	self.draw_quadtree_ = false
	
	self.pencil_ = vec4()
	self.pencil_color_ = root.Color.Red
	self.pencil_size_ = 10
	self.draw_pencil_ = false
end

function planet:rebuild_sectors_cloud()

	self.sectors_cloud_:clear()
	for i = 1, table.getn(self.loaded_sectors_) do	
		local deformed = self.loaded_sectors_[i]:get_deformed_position() * self.sectors_cloud_scale_
		self.sectors_cloud_:add_point(deformed)
	end
	
	self.sectors_cloud_:build()
end

function planet:build(radius, max_level, seed)

	self.radius_ = radius
	self.max_level_ = max_level
	self.seed_ = seed
	self.sectors_cloud_scale_ = (1.0 / (radius + 10.0)) * 10.0
	self.faces_ = {}
	
	self.atmosphere_ = atmosphere()
    self.atmosphere_:generate()
	self.clouds_ = volumetric_clouds()
	self.ocean_ = ocean()
	self.objects_ = {}
	
    self.sun_ = directional_light(self, 4, 4096)
    table.insert(self.objects_, self.sun_)
	
	self.global_envprobe_ = envprobe(self)
	self.global_envprobe_:set_size(128)
	table.insert(self.objects_, self.global_envprobe_)
	
    for i = 1, 6 do
        self.faces_[i] = root.Terrain(radius, max_level, i - 1)
		self.faces_[i].use_tessellated_patch = true
		self.faces_[i].visible_ = true
		
        -- apply spherical deformation
        self.faces_[i].spherical_deformation_ = root.SphericalDeformation(radius)
        self.faces_[i]:set_deformation(self.faces_[i].spherical_deformation_)
		
        -- apply default tile producer
        self.faces_[i].producer_ = planet_producer(self, self.faces_[i], seed)
        self.faces_[i]:add_producer(self.faces_[i].producer_)
		
		root.connect(self.faces_[i].on_page_in, function(tile)		
			if tile.level == (max_level - 2) then
			
				local deformed = tile.owner:get_face_matrix() * self.faces_[i].spherical_deformation_:local_to_deformed(tile:get_center())
				deformed = deformed * self.sectors_cloud_scale_
				
				local matches = self.sectors_cloud_:radius_search(deformed, 0.01)
				for i = 1, table.getn(matches) do
				
                    if self.active_sectors_[i] == nil then
                        self.active_sectors_[i] = self.loaded_sectors_[matches[i]]
						self.active_sectors_[i]:parse()
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
end

function planet:parse()

	local planet_object = json.parse(FileSystem:search(self.path_, true))	
	
	self.radius_ = planet_object:get_number("radius")
	self.max_level_ = planet_object:get_number("max_level")
	self.seed_ = planet_object:get_number("seed")	
	self.name = planet_object:get_string("name")
	
	self:build(self.radius_, self.max_level_, self.seed_)
	
    local sectors_array = planet_object:get_array("sectors")
    for i = 1, sectors_array.size do
	
		local sector_object = sectors_array:get(i - 1)		
		local sector = planet_sector(sector_object:get_string("path"), self)
		sector.coord_.x = sector_object:get_number("cx")
		sector.coord_.y = sector_object:get_number("cy")
		sector.coord_.z = sector_object:get_number("cz")
		sector.face_ = sector_object:get_number("face")
		sector.name = sector_object:get_string("name")
		
		table.insert(self.loaded_sectors_, sector)
	end
	
	self:rebuild_sectors_cloud()
end

function planet:write()

	local planet_object = json.Object()
	
	planet_object:set_number("radius", self.radius_)  
	planet_object:set_number("max_level", self.max_level_)  
    planet_object:set_number("seed", self.seed_)
	planet_object:set_string("name", self.name)
	
    local sectors_array = json.Array()   
    for i = 1, table.getn(self.loaded_sectors_) do
	
        local sector_object = json.Object()
		sector_object:set_string("path", self.loaded_sectors_[i].path_)
		sector_object:set_number("cx", self.loaded_sectors_[i].coord_.x)
		sector_object:set_number("cy", self.loaded_sectors_[i].coord_.y)
		sector_object:set_number("cz", self.loaded_sectors_[i].coord_.z)
		sector_object:set_number("face", self.loaded_sectors_[i].face_)
		sector_object:set_string("name", self.loaded_sectors_[i].name)
        sectors_array:push(sector_object)
	end
	
	planet_object:set_array("sectors", sectors_array)   
    local data = json.Value(planet_object):serialize(true)
    FileSystem:write(self.path_, data)
	
	for i = 1, 6 do
		self.faces_[i].producer_:write_modified_data()
	end
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
	
	-- this can be seperated from the drawing calls
	self.global_envprobe_:generate_reflection()
end

function planet:draw(camera)

	local clip = camera.proj_matrix * vec4(0, 0, -self.deferred_dist_, 1)
	self.deferred_dist_clip_space_ = (clip / clip.w).z	
	clip = camera.proj_matrix * vec4(0, 0, -self.details_dist_, 1)
	self.details_dist_clip_space_ = (clip / clip.w).z		
	clip = camera.proj_matrix * vec4(0, 0, -self.parallax_dist_, 1)
	self.parallax_dist_clip_space_ = (clip / clip.w).z
	
	local shader = self.enable_logz_ and self.logz_shader_ or self.shader_
	shader:bind()
	
	root.Shader.get():uniform("u_WorldCamPosition", camera.position)	
	root.Shader.get():uniform("u_CameraClip", camera.clip)
	root.Shader.get():uniform("u_Spherical", self.spherical_ and 1.0 or 0.0)
	root.Shader.get():uniform("u_SunDirection", self.sun_:get_direction())
	
	root.Shader.get():uniform("u_DeferredDist", self.enable_logz_ and 1.0 or self.deferred_dist_clip_space_)
	root.Shader.get():uniform("u_DetailsDist", self.details_dist_clip_space_)
	root.Shader.get():uniform("u_ParallaxDist", self.parallax_dist_clip_space_)
	root.Shader.get():uniform("u_DrawQuadtree", self.draw_quadtree_ and 1.0 or 0.0)
	
	root.Shader.get():uniform("u_Pencil", self.pencil_)
	root.Shader.get():uniform("u_PencilColor", self.pencil_color_)
	root.Shader.get():uniform("u_DrawPencil", self.draw_pencil_ and (math.length(vec3(self.pencil_)) > 0.0 and 1.0 or 0.0) or 0.0)
	
	self.atmosphere_:bind(1000)
	self.clouds_:bind_coverage()
	
	gl.ActiveTexture(gl.TEXTURE3)
	root.Shader.get():sampler("s_AlbedoDetailTexArray", 3)
	self.albedo_detail_tex_array_:bind()
	
	gl.ActiveTexture(gl.TEXTURE4)
	root.Shader.get():sampler("s_NormalDetailTexArray", 4)
	self.normal_detail_tex_array_:bind()
	
	for i = 1, 6 do
		if self.faces_[i].visible_ then
			self.faces_[i]:draw(camera)
		end
	end
	
	root.Shader.pop_stack()
	
	for i = 1, table.getn(self.active_sectors_) do
		self.active_sectors_[i]:draw(camera)
	end
	
	if self.spherical_ and not self.enable_logz_ then
		self.ocean_:draw(camera, self.radius_)
	end
end