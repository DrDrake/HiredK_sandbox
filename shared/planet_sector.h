--[[
- @file planet_sector.h
- @brief
]]

class 'planet_sector' (object_base)

function planet_sector:__init(path, planet)
    object_base.__init(self, planet)
	self.path_ = path
	self.face_ = 1
	self.paged_in_ = false
	self.local_camera_ = root.Camera()
	self.objects_ = {}
end

function planet_sector:get_deformed_position()

	local face = self.planet_.faces_[self.face_]
	return face:get_face_matrix() * face:get_deformation():local_to_deformed(self.coord_)
end

function planet_sector:get_position()

	local face = self.planet_.faces_[self.face_]
	return face:get_face_matrix() * self.coord_
end

function planet_sector:create_object_from_type(type_id)

	local new_object = nil	
	
	-- shapes
	if type_id == ID_BOX_OBJECT       then new_object =       box_object(self.planet_) end
	if type_id == ID_SPHERE_OBJECT    then new_object =    sphere_object(self.planet_) end
	if type_id == ID_CYLINDER_OBJECT  then new_object =  cylinder_object(self.planet_) end
	if type_id == ID_CONE_OBJECT      then new_object =      cone_object(self.planet_) end
	if type_id == ID_CAPSULE_OBJECT   then new_object =   capsule_object(self.planet_) end
	
	-- lights
	if type_id == ID_POINT_LIGHT      then new_object =      point_light(self.planet_) end
	if type_id == ID_ENVPROBE         then new_object =         envprobe(self.planet_) end
	
	-- vehicles
	if type_id == ID_CAR_OBJECT       then new_object =       car_object(self.planet_) end
	
	-- misc
	if type_id == ID_MODEL_OBJECT     then new_object =     model_object(self.planet_) end
	if type_id == ID_TREE_OBJECT      then new_object =      tree_object(self.planet_) end
	if type_id == ID_PARTICLE_EMITTER then new_object = particle_emitter(self.planet_) end
	
	table.insert(self.objects_, new_object)
	return new_object
end

function planet_sector:parse()

	if not self.paged_in_ then
	
		local sector_object = json.parse(FileSystem:search(self.path_, true))	
		local objects_array = sector_object:get_array("objects")
		for i = 1, objects_array.size do
		
			local object_object = objects_array:get(i - 1)			
			local object = self:create_object_from_type(object_object:get_number("type_id"))
			object:parse(object_object)
		end
		
		print(string.format("Paged In '%s' sector", self.name))
		self.paged_in_ = true
	end
end

function planet_sector:write()

	local sector_object = json.Object()	
	
    local objects_array = json.Array()
    for i = 1, table.getn(self.objects_) do
    
        local object_object = json.Object()
		object_object:set_number("type_id", self.objects_[i].type_id_)
        self.objects_[i]:write(object_object)
        objects_array:push(object_object)
    end
    
    sector_object:set_array("objects", objects_array)	
	local data = json.Value(sector_object):serialize(true)
	FileSystem:write(self.path_, data)
end

function planet_sector:draw(camera)

	self.local_camera_:copy(camera)
	self.local_camera_.position = camera.position - self:get_deformed_position()
	self.local_camera_:refresh()
	
	local offset_camera = self.planet_.spherical_ and self.local_camera_ or camera
	root.Shader.get():uniform("u_DeferredDist", self.planet_.enable_logz_ and 1.0 or self.planet_.deferred_dist_clip_space_)
	root.Shader.get():uniform("u_WorldCamPosition", offset_camera.position)
	root.Shader.get():uniform("u_SunDirection", self.planet_.sun_:get_direction())
	self.planet_.atmosphere_:bind(1000)

    for i = 1, table.getn(self.objects_) do
        self.objects_[i]:draw(offset_camera)
    end
	
	-- TEMP!!!
	for i = 1, table.getn(self.planet_.objects_) do
		self.planet_.objects_[i]:draw(offset_camera)
	end
end