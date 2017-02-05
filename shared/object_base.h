--[[
- @file object_base.h
- @brief
]]

--[[
////////////////////////////////////////////////////////////////////////////////
// object_base implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'object_base' (root.ScriptObject)

function object_base:__init(planet)
    root.ScriptObject.__init(self, "object")
	self.type = root.ScriptObject.Dynamic
	self.type_id_ = ID_BASE_OBJECT
	
    self.planet_ = planet
	self.coord_ = vec3()
	self.transform_ = root.Transform()
	self.mesh_ = nil
	
	self.visible_ = true
	self.write_depth_ = true
	self.is_light_ = false
	
	self.rigid_body_ = nil
	self.rigid_body_support_ = false
	self.rigid_body_enabled_ = false
	self.rigid_body_mass_ = 0	
	self.replica_ = nil
end

function object_base:parse(object)

	self.name = object:get_string("name")
    self.transform_.position = object:get_vec3("position")
	self.transform_.rotation = object:get_quat("rotation")
	self.transform_.scale = object:get_vec3("scale")
	
	self.rigid_body_enabled_ = object:get_bool("rigid_body_enabled")
	if self.rigid_body_enabled_ then
		self.rigid_body_mass_ = object:get_number("rigid_body_mass")
	end
end

function object_base:write(object)

	object:set_string("name", self.name)
	object:set_vec3("position", self.transform_.position)
	object:set_quat("rotation", self.transform_.rotation)
	object:set_vec3("scale", self.transform_.scale)
	
	object:set_bool("rigid_body_enabled", self.rigid_body_enabled_)
	if self.rigid_body_enabled_ then
		object:set_number("rigid_body_mass", self.rigid_body_mass_)
	end
end

function object_base:on_network_start(network)
end

function object_base:on_physics_start(dynamic_world)
end

function object_base:__update(dt)

	if self.rigid_body_ ~= nil then
		self.transform_:copy(self.rigid_body_)
	end
end

function object_base:draw(camera)

	if self.visible_ and self.mesh_ ~= nil then
		if not self.write_depth_ then
			gl.DepthMask(false)
		end
		
		self.mesh_:copy(self.transform_)
		self.mesh_:draw(camera)
		gl.DepthMask(true)
	end
end

function object_base:draw_shadow(matrix)

    if self.mesh_ ~= nil and self.visible_ then
        root.Shader.get():uniform("u_ViewProjMatrix", matrix)
        self.mesh_:copy(self.transform_)
        self.mesh_:draw()
    end
end

function object_base:draw_transparent(camera)
end

function object_base:draw_light(camera)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// object_base_net implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'object_base_net' (root.NetObject)

function object_base_net:__init(network, object)
	root.NetObject.__init(self, network)
	self.type = root.ScriptObject.Dynamic
	self.object_ = object
	
	if not network.autoritative then
		self.history_ = root.TransformHistory(10, 1000)
	end
end

function object_base_net:__write_allocation_id(connection, bs)

	root.NetObject.__write_allocation_id(self, connection, bs) -- call first
	bs:write_integer(self.object_.type_id_)
end

function object_base_net:__serialize_construction(bs, connection)

	bs:write_vec3(self.object_.transform_.position)
	bs:write_quat(self.object_.transform_.rotation)
	root.NetObject.__serialize_construction(self, bs, connection)
end

function object_base_net:__deserialize_construction(bs, connection)

	self.object_.transform_.position = bs:read_vec3()
	self.object_.transform_.rotation = bs:read_quat()
	return root.NetObject.__deserialize_construction(self, bs, connection)
end

function object_base_net:__serialize(parameters)

	parameters:get_bitstream(0):write_vec3(self.object_.transform_.position)
	parameters:get_bitstream(0):write_quat(self.object_.transform_.rotation)
	return root.Network.BROADCAST_IDENTICALLY
end

function object_base_net:__deserialize(parameters)

	self.object_.transform_.position = parameters:get_bitstream(0):read_vec3()
	self.object_.transform_.rotation = parameters:get_bitstream(0):read_quat()
	self.history_:write(self.object_.transform_)
end

function object_base_net:__update(dt)

	if not self.network.autoritative then
		self.history_:read(self.object_.transform_, self.network.auto_serialize_interval)
	end
end