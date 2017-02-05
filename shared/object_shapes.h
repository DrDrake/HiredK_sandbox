--[[
- @file object_shapes.h
- @brief
]]

--[[
////////////////////////////////////////////////////////////////////////////////
// box_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'box_object' (object_base)

function box_object:__init(planet)
    object_base.__init(self, planet)
	self.type_id_ = ID_BOX_OBJECT
	self.rigid_body_support_ = true
	
	self.extent_ = vec3(0.5)
	self:rebuild_mesh()
end

function box_object:rebuild_mesh()
	self.mesh_ = root.Mesh.build_box(self.extent_)
	self.transform_.box:set(self.extent_)
end

function box_object:set_extent(value)
	self.extent_ = value
	self:rebuild_mesh()
end

function box_object:parse(object)

    self.extent_ = object:get_vec3("extent")
	self:rebuild_mesh()
	
	object_base.parse(self, object)
end

function box_object:write(object)

	object:set_vec3("extent", self.extent_)
	object_base.write(self, object)
end

function box_object:on_physics_start(dynamic_world)

	if self.rigid_body_enabled_ and self.rigid_body_ == nil then
		self.rigid_body_ = bt.RigidBody.create_box(dynamic_world, self.transform_, self.extent_, self.rigid_body_mass_)
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// sphere_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'sphere_object' (object_base)

function sphere_object:__init(planet)
    object_base.__init(self, planet)
	self.type_id_ = ID_SPHERE_OBJECT
	self.rigid_body_support_ = true
	
	self.radius_ = 0.5
	self.segments_ = 16
	self:rebuild_mesh()
end

function sphere_object:rebuild_mesh()
	self.mesh_ = root.Mesh.build_sphere(self.radius_, self.segments_, self.segments_)
	self.transform_.box:set(vec3(self.radius_))
end

function sphere_object:set_radius(value)
	self.radius_ = value
	self:rebuild_mesh()
end

function sphere_object:set_segments(value)
	self.segments_ = value
	self:rebuild_mesh()
end

function sphere_object:parse(object)

    self.radius_ = object:get_number("radius")
	self.segments_ = object:get_number("segments")
	self:rebuild_mesh()
	
	object_base.parse(self, object)
end

function sphere_object:write(object)

	object:set_number("radius", self.radius_)
	object:set_number("segments", self.segments_)
	object_base.write(self, object)
end

function sphere_object:on_physics_start(dynamic_world)

	if self.rigid_body_enabled_ and self.rigid_body_ == nil then
		self.rigid_body_ = bt.RigidBody.create_sphere(dynamic_world, self.transform_, self.radius_, self.rigid_body_mass_)
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// cylinder_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'cylinder_object' (object_base)

function cylinder_object:__init(planet)
    object_base.__init(self, planet)
	self.type_id_ = ID_CYLINDER_OBJECT
	self.rigid_body_support_ = true
	
	self.radius_ = 0.5
	self.height_ = 1.0
	self.radial_segments_ = 32
	self.height_segments_ = 1
	self:rebuild_mesh()
end

function cylinder_object:rebuild_mesh()
	self.mesh_ = root.Mesh.build_cylinder(self.radius_, self.height_, self.radial_segments_, self.height_segments_)
	self.transform_.box:set(vec3(self.radius_, self.height_ * 0.5, self.radius_))
end

function cylinder_object:set_radius(value)
	self.radius_ = value
	self:rebuild_mesh()
end

function cylinder_object:set_height(value)
	self.height_ = value
	self:rebuild_mesh()
end

function cylinder_object:set_radial_segments(value)
	self.radial_segments_ = value
	self:rebuild_mesh()
end

function cylinder_object:set_height_segments(value)
	self.height_segments_ = value
	self:rebuild_mesh()
end

function cylinder_object:parse(object)

    self.radius_ = object:get_number("radius")
	self.height_ = object:get_number("height")
    self.radial_segments_ = object:get_number("radial_segments")
	self.height_segments_ = object:get_number("height_segments")
	self:rebuild_mesh()
	
	object_base.parse(self, object)
end

function cylinder_object:write(object)

	object:set_number("radius", self.radius_)
	object:set_number("height", self.height_)
	object:set_number("radial_segments", self.radial_segments_)
	object:set_number("height_segments", self.height_segments_)
	object_base.write(self, object)
end

function cylinder_object:on_physics_start(dynamic_world)

	if self.rigid_body_enabled_ and self.rigid_body_ == nil then
		self.rigid_body_ = bt.RigidBody.create_cylinder(dynamic_world, self.transform_, self.radius_, self.height_, self.rigid_body_mass_)
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// cone_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'cone_object' (object_base)

function cone_object:__init(planet)
    object_base.__init(self, planet)
	self.type_id_ = ID_CONE_OBJECT
	self.rigid_body_support_ = true	
	
	self.radius_ = 0.5
	self.height_ = 1.0
	self.segments_ = 16
	self:rebuild_mesh()
end

function cone_object:rebuild_mesh()
	self.mesh_ = root.Mesh.build_cone(self.radius_, self.height_, self.segments_)
	self.transform_.box:set(vec3(self.radius_, self.height_ * 0.5, self.radius_))
end

function cone_object:set_radius(value)
	self.radius_ = value
	self:rebuild_mesh()
end

function cone_object:set_height(value)
	self.height_ = value
	self:rebuild_mesh()
end

function cone_object:set_segments(value)
	self.segments_ = value
	self:rebuild_mesh()
end

function cone_object:parse(object)

    self.radius_ = object:get_number("radius")
	self.height_ = object:get_number("height")
    self.segments_ = object:get_number("segments")
	self:rebuild_mesh()
	
	object_base.parse(self, object)
end

function cone_object:write(object)

	object:set_number("radius", self.radius_)
	object:set_number("height", self.height_)
	object:set_number("segments", self.segments_)
	object_base.write(self, object)
end

function cone_object:on_physics_start(dynamic_world)

	if self.rigid_body_enabled_ and self.rigid_body_ == nil then
		self.rigid_body_ = bt.RigidBody.create_cone(dynamic_world, self.transform_, self.radius_, self.height_, self.rigid_body_mass_)
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// capsule_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'capsule_object' (object_base)

function capsule_object:__init(planet)
    object_base.__init(self, planet)
	self.type_id_ = ID_CAPSULE_OBJECT
	self.rigid_body_support_ = true	
	
	self.radius_ = 0.5
	self.height_ = 1.0
	self.segments_ = 16
	self:rebuild_mesh()
end

function capsule_object:rebuild_mesh()
	self.mesh_ = root.Mesh.build_capsule(self.radius_, self.height_, self.segments_)
	self.transform_.box:set(vec3(self.radius_, self.height_, self.radius_))
end

function capsule_object:set_radius(value)
	self.radius_ = value
	self:rebuild_mesh()
end

function capsule_object:set_height(value)
	self.height_ = value
	self:rebuild_mesh()
end

function capsule_object:set_segments(value)
	self.segments_ = value
	self:rebuild_mesh()
end

function capsule_object:parse(object)

    self.radius_ = object:get_number("radius")
	self.height_ = object:get_number("height")
    self.segments_ = object:get_number("segments")
	self:rebuild_mesh()
	
	object_base.parse(self, object)
end

function capsule_object:write(object)

	object:set_number("radius", self.radius_)
	object:set_number("height", self.height_)
	object:set_number("segments", self.segments_)
	object_base.write(self, object)
end

function capsule_object:on_physics_start(dynamic_world)

	if self.rigid_body_enabled_ and self.rigid_body_ == nil then
		self.rigid_body_ = bt.RigidBody.create_capsule(dynamic_world, self.transform_, self.radius_, self.height_, self.rigid_body_mass_)
	end
end