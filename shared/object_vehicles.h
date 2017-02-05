--[[
- @file object_vehicles.h
- @brief
]]

--[[
////////////////////////////////////////////////////////////////////////////////
// car_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'car_object' (object_base)

function car_object:__init(planet)
	object_base.__init(self, planet)
	self.type_id_ = ID_CAR_OBJECT
	
	self.last_direction_ = vec2()
	self.last_action0_ = false -- leave
end

function car_object:on_network_start(network)

	if not network.autoritative then	
		self.mesh_ = root.Mesh.build_box(vec3(1, 1, 1))
		self.transform_.scale = vec3(1, 0.6, 2)
	end
	
	self.replica_ = object_base_net(network, self)
end

function car_object:on_physics_start(dynamic_world)

	if self.rigid_body_ == nil then	
		self.rigid_body_ = bt.RigidBody.create_box(dynamic_world, self.transform_, vec3(1, 0.6, 2), 1000.0)
		self.controller_ = bt.VehiculeController(dynamic_world, self.rigid_body_)
	end
end

function car_object:process_input(camera)

	local direction = vec2()
	local action0 = false

	if sf.Keyboard.is_key_pressed(sf.Keyboard.W) then
		direction = direction + vec2(1, 0)
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.S) then
		direction = direction - vec2(1, 0)
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.D) then
		direction = direction - vec2(0, 1)
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.A) then
		direction = direction + vec2(0, 1)
	end
	
	if sf.Keyboard.is_key_pressed(sf.Keyboard.E) then
		action0 = true
	end	
	
	if math.length(self.last_direction_ - direction) > 0.0 or
		self.last_action0_ ~= action0 then
		
		local bs = root.BitStream()
		bs:write_message_id(MessageID.ID_PLAYER_INPUT)
		bs:write_network_id(self.replica_.network_id)
		bs:write_vec2(direction)
		bs:write_bool(action0)
		
		self.replica_.network:send(bs)
	end
	
	self.last_direction_ = direction
	self.last_action0_ = action0
end

function car_object:receive_input(bs)

	local direction = bs:read_vec2()
	local action0 = bs:read_bool()
	
	print(direction.x, direction.y)
	
	self.controller_:set_steering(direction.y)
	self.controller_:set_accelerator(direction.x)
end