--[[
- @file player.h
- @brief
]]

class 'player' (object_base)

function player:__init(planet)
	object_base.__init(self, planet)
	self.type_id_ = ID_PLAYER_OBJECT
	
	self.last_direction_ = vec3()
	self.last_action0_ = false -- running
	self.last_action1_ = false -- interact
	self.last_action2_ = false -- jumping
	
	self.current_animation_index_ = 0
	self.vehicle_network_id_ = 0
	self.orientation_ = quat()
end

function player:on_network_start(network)

	if not network.autoritative then
	
		self.mesh_ = root.Animator()
		self.mesh_:load_skeleton(FileSystem:search("animations/soldier_skeleton.ozz", true))
		self.mesh_:load_mesh(FileSystem:search("animations/soldier_mesh.ozz", true))
		self.mesh_:add_animation(0, FileSystem:search("animations/soldier_anim_idle.ozz", true))
		self.mesh_:add_animation(1, FileSystem:search("animations/soldier_anim_walk.ozz", true))
		self.mesh_:add_animation(2, FileSystem:search("animations/soldier_anim_run.ozz", true))
		self.transform_.scale = vec3(1.5)
	end

	self.replica_ = player_net(network, self)
end

function player:on_physics_start(dynamic_world)

	if self.rigid_body_ == nil then	
		self.rigid_body_ = bt.CharacterController(dynamic_world, 1.5, 0.2)
		self.rigid_body_:wrap(vec3(0, 50, 0))
	end
end

function player:is_inside_vehicle()
	return self.vehicle_network_id_ ~= 0
end

function player:get_camera_target()
	if self:is_inside_vehicle() then
		local replica = self.replica_.network:get_object_from_id(self.vehicle_network_id_)
		return replica.object_.transform_.position
	end
	
	return self.transform_.position + vec3(0, 2, 0)
end

function player:process_input(camera)

	if self:is_inside_vehicle() then
		local replica = self.replica_.network:get_object_from_id(self.vehicle_network_id_)
		replica.object_:process_input(camera)
		return
	end

	local direction = vec3()
	local action0 = false
	local action1 = false
	local action2 = false
	
	if sf.Keyboard.is_key_pressed(sf.Keyboard.W) then
		direction = direction - vec3(math.column(camera.local_, 2))
	end		
	if sf.Keyboard.is_key_pressed(sf.Keyboard.S) then
		direction = direction + vec3(math.column(camera.local_, 2))
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.D) then
		direction = direction + vec3(math.column(camera.local_, 0))
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.A) then
		direction = direction - vec3(math.column(camera.local_, 0))
	end
	
	if sf.Keyboard.is_key_pressed(sf.Keyboard.LShift) then
		action0 = true
	end	
	if sf.Keyboard.is_key_pressed(sf.Keyboard.E) then
		action1 = true
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.Space) then
		action2 = true
	end
	
	if math.length(self.last_direction_ - direction) > 0.0 or
	   self.last_action0_ ~= action0 or
	   self.last_action1_ ~= action1 or
	   self.last_action2_ ~= action2 then
	   
		local bs = root.BitStream()
		bs:write_message_id(MessageID.ID_PLAYER_INPUT)
		bs:write_network_id(self.replica_.network_id)
		bs:write_vec3(direction)
		bs:write_bool(action0)
		bs:write_bool(action1)
		bs:write_bool(action2)
		
		self.replica_.network:send(bs)
	end
	
	self.last_direction_ = direction
	self.last_action0_ = action0
	self.last_action1_ = action1
	self.last_action2_ = action2
end

function player:receive_input(bs)

	local direction = bs:read_vec3()
	local action0 = bs:read_bool()
	local action1 = bs:read_bool()
	local action2 = bs:read_bool()
	
	if math.length(direction) > 0 then
		self.orientation_ = math.conjugate(quat(math.lookat(vec3(direction.x, 0, direction.z), vec3(), vec3(0, 1, 0))))
		self.current_animation_index_ = action0 and 2 or 1
	else
		self.current_animation_index_ = 0
	end
	
	if action1 then
		for i = 1, table.getn(self.planet_.objects_) do
		
			if self.planet_.objects_[i].type_id_ == ID_CAR_OBJECT then
				local dist = math.distance(self.transform_.position, self.planet_.objects_[i].transform_.position)
				if dist < 5.0 then
					self.vehicle_network_id_ = self.planet_.objects_[i].replica_.network_id
					print("Enter Vehicle:", self.vehicle_network_id_)
				end
			end
		end
	end
	
	if action2 then
		self.rigid_body_:jump(action0 and 25.0 or 15.0)
	end
	
	self.rigid_body_:set_walk_direction(direction * (action0 and 0.85 or 0.15))
end

function player:__update(dt)

	object_base.__update(self, dt)
	
	if self.replica_.network.autoritative then
		self.transform_.position.y = self.rigid_body_.position.y - 1.5
		self.transform_.rotation = math.normalize(self.orientation_)
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// player_net implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'player_net' (object_base_net)

function player_net:__init(network, object)
	object_base_net.__init(self, network, object)
end

function player_net:__serialize(parameters)

	parameters:get_bitstream(0):write_integer(self.object_.current_animation_index_)
	parameters:get_bitstream(0):write_bool(self.object_:is_inside_vehicle())
	if self.object_:is_inside_vehicle() then
		parameters:get_bitstream(0):write_network_id(self.object_.vehicle_network_id_)
	end
	
	return object_base_net.__serialize(self, parameters)
end

function player_net:__deserialize(parameters)

	self.object_.mesh_.current_animation_index = parameters:get_bitstream(0):read_integer()
	if parameters:get_bitstream(0):read_bool() then
		self.object_.vehicle_network_id_ = parameters:get_bitstream(0):read_network_id()
	else
		self.object_.vehicle_network_id_ = 0
	end
	
	object_base_net.__deserialize(self, parameters)
end