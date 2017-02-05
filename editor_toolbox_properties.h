--[[
- @file editor_toolbox_properties.h
- @brief
]]

class 'editor_toolbox_properties' (gwen.PropertyTree)

function editor_toolbox_properties:__init(editor, parent)
	gwen.PropertyTree.__init(self, parent)
	self.editor_ = editor
	
	-- basics
	self:init_transform_properties()
	self:init_rigid_body_properties()
	
	-- shapes
	self:init_box_properties()
	self:init_sphere_properties()
	self:init_cylinder_properties()
	self:init_cone_properties()
	self:init_capsule_properties()
	
	-- lights
	self:init_point_light_properties()
	self:init_envprobe_properties()
	
	self:expand_all()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// basics properties implementation:
////////////////////////////////////////////////////////////////////////////////
]]

function editor_toolbox_properties:init_transform_properties()

	self.transform_props_ = self:add(gwen.Properties(self), "Transform")	
	self.transform_position_ = self.transform_props_:add_vec3(gwen.PropertyRow(self), "Position", vec3(0, 0, 0))
	root.connect(self.transform_position_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1].transform_.position = control.property.value
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)	
	self.transform_rotation_ = self.transform_props_:add_vec3(gwen.PropertyRow(self), "Rotation", vec3(0, 0, 0))
	root.connect(self.transform_rotation_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1].transform_.rotation = quat(math.radians(control.property.value))
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)		
	self.transform_scale_ = self.transform_props_:add_vec3(gwen.PropertyRow(self), "Scale", vec3(1, 1, 1))
	root.connect(self.transform_scale_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1].transform_.scale = control.property.value
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)
end

function editor_toolbox_properties:init_rigid_body_properties()

	self.rigid_body_props_ = self:add(gwen.Properties(self), "Rigid Body")
	self.rigid_body_enabled_ = self.rigid_body_props_:add_checkbox(gwen.PropertyRow(self), "Enabled", false)
	root.connect(self.rigid_body_enabled_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1].rigid_body_enabled_ = control.property.value
		end
	end)	
	self.rigid_body_mass_ = self.rigid_body_props_:add_numeric(gwen.PropertyRow(self), "Mass", 0)
	root.connect(self.rigid_body_mass_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1].rigid_body_mass_ = control.property.value
		end
	end)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// shapes properties implementation:
////////////////////////////////////////////////////////////////////////////////
]]

function editor_toolbox_properties:init_box_properties()

	self.box_props_ = self:add(gwen.Properties(self), "Box")
	self.box_extent_ = self.box_props_:add_vec3(gwen.PropertyRow(self), "Extent", vec3(0.5))
	root.connect(self.box_extent_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_extent(control.property.value)
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)
end

function editor_toolbox_properties:init_sphere_properties()

	self.sphere_props_ = self:add(gwen.Properties(self), "Sphere")
	self.sphere_radius_ = self.sphere_props_:add_numeric(gwen.PropertyRow(self), "Radius", 0.5)
	root.connect(self.sphere_radius_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_radius(control.property.value)
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)
	self.sphere_segments_ = self.sphere_props_:add_numeric_updown(gwen.PropertyRow(self), "Segments", 16)
	root.connect(self.sphere_segments_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_segments(control.property.value)
		end
	end)
end

function editor_toolbox_properties:init_cylinder_properties()

	self.cylinder_props_ = self:add(gwen.Properties(self), "Cylinder")
	self.cylinder_radius_ = self.cylinder_props_:add_numeric(gwen.PropertyRow(self), "Radius", 0.5)
	root.connect(self.cylinder_radius_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_radius(control.property.value)
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)
	self.cylinder_height_ = self.cylinder_props_:add_numeric(gwen.PropertyRow(self), "Height", 1.0)
	root.connect(self.cylinder_height_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_height(control.property.value)
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)
	self.cylinder_radial_segments_ = self.cylinder_props_:add_numeric_updown(gwen.PropertyRow(self), "Radial Segments", 32)
	root.connect(self.cylinder_radial_segments_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_radial_segments(control.property.value)
		end
	end)
	self.cylinder_height_segments_ = self.cylinder_props_:add_numeric_updown(gwen.PropertyRow(self), "Height Segments", 16)
	root.connect(self.cylinder_height_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_height_segments(control.property.value)
		end
	end)
end

function editor_toolbox_properties:init_cone_properties()

	self.cone_props_ = self:add(gwen.Properties(self), "Cone")
	self.cone_radius_ = self.cone_props_:add_numeric(gwen.PropertyRow(self), "Radius", 0.5)
	root.connect(self.cone_radius_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_radius(control.property.value)
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)
	self.cone_height_ = self.cone_props_:add_numeric(gwen.PropertyRow(self), "Height", 1.0)
	root.connect(self.cone_height_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_height(control.property.value)
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)
	self.cone_segments_ = self.cone_props_:add_numeric_updown(gwen.PropertyRow(self), "Segments", 16)
	root.connect(self.cone_segments_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_segments(control.property.value)
		end
	end)
end

function editor_toolbox_properties:init_capsule_properties()

	self.capsule_props_ = self:add(gwen.Properties(self), "Capsule")
	self.capsule_radius_ = self.capsule_props_:add_numeric(gwen.PropertyRow(self), "Radius", 0.5)
	root.connect(self.capsule_radius_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_radius(control.property.value)
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)
	self.capsule_height_ = self.capsule_props_:add_numeric(gwen.PropertyRow(self), "Height", 1.0)
	root.connect(self.capsule_height_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_height(control.property.value)
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)
	self.capsule_segments_ = self.capsule_props_:add_numeric_updown(gwen.PropertyRow(self), "Segments", 16)
	root.connect(self.capsule_segments_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_segments(control.property.value)
		end
	end)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// lights properties implementation:
////////////////////////////////////////////////////////////////////////////////
]]

function editor_toolbox_properties:init_point_light_properties()

	self.point_light_props_ = self:add(gwen.Properties(self), "Point Light")
	self.point_light_radius_ = self.point_light_props_:add_numeric(gwen.PropertyRow(self), "Radius", 15)
	root.connect(self.point_light_radius_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_radius(control.property.value)
			self.editor_.selection_:rebuild_selection_bbox()
		end
	end)
	self.point_light_intensity_ = self.point_light_props_:add_numeric_updown(gwen.PropertyRow(self), "Intensity", 50, 1, 50000)
	root.connect(self.point_light_intensity_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1].intensity_ = control.property.value
		end
	end)
end

function editor_toolbox_properties:init_envprobe_properties()

	self.envprobe_props_ = self:add(gwen.Properties(self), "Envprobe")
	self.envprobe_size_ = self.envprobe_props_:add_combobox(gwen.PropertyRow(self), "Size", { "8", "16", "32", "64", "128", "256", "512", "1024" }, "64")
	root.connect(self.envprobe_size_.on_change, function(control)
		if table.getn(self.editor_.selection_.selected_objects_) == 1 then
			self.editor_.selection_.selected_objects_[1]:set_size(tonumber(control.property.value))
		end
	end)
end

function editor_toolbox_properties:on_selection_update()

	self.rigid_body_props_:hide()
	self.box_props_:hide()
	self.sphere_props_:hide()
	self.cylinder_props_:hide()
	self.cone_props_:hide()
	self.capsule_props_:hide()
	self.point_light_props_:hide()
	self.envprobe_props_:hide()

	if table.getn(self.editor_.selection_.selected_objects_) == 1 then	
	
		local object = self.editor_.selection_.selected_objects_[1]			
		self.transform_position_.property.value = object.transform_.position
		self.transform_rotation_.property.value = math.degrees(math.euler_angles(object.transform_.rotation))
		self.transform_scale_.property.value = object.transform_.scale
		
		if object.rigid_body_support_ then
			self.rigid_body_props_:show()
			self.rigid_body_enabled_.property.value = object.rigid_body_enabled_
			self.rigid_body_mass_.property.value = object.rigid_body_mass_
		end
		
		if object.type_id_ == ID_BOX_OBJECT then
			self.box_props_:show()
			self.box_extent_.property.value = object.extent_
		end
		
		if object.type_id_ == ID_SPHERE_OBJECT then
			self.sphere_props_:show()
			self.sphere_radius_.property.value = object.radius_
			self.sphere_segments_.property.value = object.segments_
		end
		
		if object.type_id_ == ID_CYLINDER_OBJECT then
			self.cylinder_props_:show()
			self.cylinder_radius_.property.value = object.radius_
			self.cylinder_height_.property.value = object.height_
			self.cylinder_radial_segments_.property.value = object.radial_segments_
			self.cylinder_height_segments_.property.value = object.height_segments_
		end
		
		if object.type_id_ == ID_CONE_OBJECT then
			self.cone_props_:show()
			self.cone_radius_.property.value = object.radius_
			self.cone_height_.property.value = object.height_
			self.cone_segments_.property.value = object.segments_
		end
		
		if object.type_id_ == ID_CAPSULE_OBJECT then
			self.capsule_props_:show()
			self.capsule_radius_.property.value = object.radius_
			self.capsule_height_.property.value = object.height_
			self.capsule_segments_.property.value = object.segments_
		end
		
		if object.type_id_ == ID_POINT_LIGHT then
			self.point_light_props_:show()
			self.point_light_radius_.property.value = object.radius_
			self.point_light_intensity_.property.value = object.intensity_
		end
		
		if object.type_id_ == ID_ENVPROBE then
			self.envprobe_props_:show()
			self.envprobe_size_.property.value = tostring(object.size_)
		end
	end
end