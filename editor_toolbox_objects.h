--[[
- @file editor_toolbox_objects.h
- @brief
]]

class 'editor_toolbox_objects' (gwen.CollapsibleList)

function editor_toolbox_objects:__init(editor, parent)
	gwen.CollapsibleList.__init(self, parent)
	self.editor_ = editor
	
	self.object_to_add_ = nil
	self.is_dragging_ = false
	self:init_shapes_category()
	self:init_lights_category()
	self:init_vehicles_category()
	self:init_misc_category()
end

function editor_toolbox_objects:spawn_object(object)

	self.editor_.toolbox_.created_objects_count_ = self.editor_.toolbox_.created_objects_count_ + 1
	self.object_to_add_ = object
	object.write_depth_ = false
	self.is_dragging_ = true
	
	self.editor_.selection_:clear()
	self.editor_.selection_:add_to_selected(object)
	self.editor_:refresh_all()
end

function editor_toolbox_objects:process_event(event)

	if event.type == sf.Event.MouseButtonReleased then
	
		if self.is_dragging_ then
			self.editor_.toolbox_.properties_tab_.page_:on_selection_update()
			self.editor_.selection_:rebuild_selection_bbox()
			self.object_to_add_.write_depth_ = true
			self.object_to_add_ = nil
			self.is_dragging_ = false
		end
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// shapes category implementation:
////////////////////////////////////////////////////////////////////////////////
]]

function editor_toolbox_objects:init_shapes_category()

	self.shapes_category_ = self:add(gwen.CollapsibleCategory(self), "Shapes")
	self:init_box_object_button()
	self:init_sphere_object_button()
	self:init_cylinder_object_button()
	self:init_cone_object_button()
	self:init_capsule_object_button()
end

function editor_toolbox_objects:init_box_object_button()

	self.box_button_ = gwen.Button(self.shapes_category_)
	self.box_button_:set_bounds(6, 24, 32, 32)
	self.box_button_:set_tooltip("Box Object")
	
	root.connect(self.box_button_.on_down, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then
		
			local object = active_tab.sector_:create_object_from_type(ID_BOX_OBJECT)
			object.name = string.format("Box %04d", self.editor_.toolbox_.created_objects_count_)	
			self:spawn_object(object)
		end
	end)
end

function editor_toolbox_objects:init_sphere_object_button()

	self.sphere_button_ = gwen.Button(self.shapes_category_)
	self.sphere_button_:set_bounds(42, 24, 32, 32)
	self.sphere_button_:set_tooltip("Sphere Object")
	
	root.connect(self.sphere_button_.on_down, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then
		
			local object = active_tab.sector_:create_object_from_type(ID_SPHERE_OBJECT)
			object.name = string.format("Sphere %04d", self.editor_.toolbox_.created_objects_count_)
			self:spawn_object(object)
		end
	end)
end

function editor_toolbox_objects:init_cylinder_object_button()

	self.cylinder_button_ = gwen.Button(self.shapes_category_)
	self.cylinder_button_:set_bounds(78, 24, 32, 32)
	self.cylinder_button_:set_tooltip("Cylinder Object")
	
	root.connect(self.cylinder_button_.on_down, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then
		
			local object = active_tab.sector_:create_object_from_type(ID_CYLINDER_OBJECT)
			object.name = string.format("Cylinder %04d", self.editor_.toolbox_.created_objects_count_)
			self:spawn_object(object)
		end
	end)
end

function editor_toolbox_objects:init_cone_object_button()

	self.cone_button_ = gwen.Button(self.shapes_category_)
	self.cone_button_:set_bounds(114, 24, 32, 32)
	self.cone_button_:set_tooltip("Cone Object")
	
	root.connect(self.cone_button_.on_down, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then
		
			local object = active_tab.sector_:create_object_from_type(ID_CONE_OBJECT)
			object.name = string.format("Cone %04d", self.editor_.toolbox_.created_objects_count_)
			self:spawn_object(object)
		end
	end)
end

function editor_toolbox_objects:init_capsule_object_button()

	self.capsule_button_ = gwen.Button(self.shapes_category_)
	self.capsule_button_:set_bounds(150, 24, 32, 32)
	self.capsule_button_:set_tooltip("Capulse Object")
	
	root.connect(self.capsule_button_.on_down, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then
		
			local object = active_tab.sector_:create_object_from_type(ID_CAPSULE_OBJECT)
			object.name = string.format("Capsule %04d", self.editor_.toolbox_.created_objects_count_)
			self:spawn_object(object)
		end
	end)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// lights category implementation:
////////////////////////////////////////////////////////////////////////////////
]]

function editor_toolbox_objects:init_lights_category()

	self.lights_category_ = self:add(gwen.CollapsibleCategory(self), "Lights")
	self:init_point_light_button()
	self:init_envprobe_button()
end

function editor_toolbox_objects:init_point_light_button()

	self.point_light_button_ = gwen.Button(self.lights_category_)
	self.point_light_button_:set_bounds(6, 24, 32, 32)
	self.point_light_button_:set_tooltip("Point Light")
	
	root.connect(self.point_light_button_.on_down, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then
		
			local object = active_tab.sector_:create_object_from_type(ID_POINT_LIGHT)
			object.name = string.format("Point Light %04d", self.editor_.toolbox_.created_objects_count_)	
			self:spawn_object(object)
		end
	end)
end

function editor_toolbox_objects:init_envprobe_button()

	self.envprobe_button_ = gwen.Button(self.lights_category_)
	self.envprobe_button_:set_bounds(42, 24, 32, 32)
	self.envprobe_button_:set_tooltip("Envprobe")
	
	root.connect(self.envprobe_button_.on_down, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then
		
			local object = active_tab.sector_:create_object_from_type(ID_ENVPROBE)
			object.name = string.format("Envprobe %04d", self.editor_.toolbox_.created_objects_count_)	
			self:spawn_object(object)
		end
	end)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// vehicles category implementation:
////////////////////////////////////////////////////////////////////////////////
]]

function editor_toolbox_objects:init_vehicles_category()

	self.vehicles_category_ = self:add(gwen.CollapsibleCategory(self), "Vehicles")
	self:init_car_object_button()
end

function editor_toolbox_objects:init_car_object_button()

	self.car_object_button_ = gwen.Button(self.vehicles_category_)
	self.car_object_button_:set_bounds(6, 24, 32, 32)
	self.car_object_button_:set_tooltip("Car")
	
	root.connect(self.car_object_button_.on_down, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then
		
			local object = active_tab.sector_:create_object_from_type(ID_CAR_OBJECT)
			object.name = string.format("Car %04d", self.editor_.toolbox_.created_objects_count_)	
			self:spawn_object(object)
		end
	end)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// misc category implementation:
////////////////////////////////////////////////////////////////////////////////
]]

function editor_toolbox_objects:init_misc_category()

	self.misc_category_ = self:add(gwen.CollapsibleCategory(self), "Misc")
	self:init_model_object_button()
	self:init_tree_object_button()
	self:init_particle_emitter_button()
end

function editor_toolbox_objects:init_model_object_button()

	self.model_button_ = gwen.Button(self.misc_category_)
	self.model_button_:set_bounds(6, 24, 32, 32)
	self.model_button_:set_tooltip("Model Object")
	
	root.connect(self.model_button_.on_down, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then
		
			local object = active_tab.sector_:create_object_from_type(ID_MODEL_OBJECT)
			object.name = string.format("Model %04d", self.editor_.toolbox_.created_objects_count_)	
			self:spawn_object(object)
		end
	end)
	
	root.connect(self.model_button_.on_up, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then		
			self.editor_:create_open_file_dialog("Open Model", { "glb" }, function(path)
				self.editor_.selection_.selected_objects_[1]:load_mesh(path)
				self.editor_.selection_:rebuild_selection_bbox()
			end)
		end
	end)
end

function editor_toolbox_objects:init_tree_object_button()

	self.tree_button_ = gwen.Button(self.misc_category_)
	self.tree_button_:set_bounds(42, 24, 32, 32)
	self.tree_button_:set_tooltip("Tree Object")
	
	root.connect(self.tree_button_.on_down, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then
		
			local object = active_tab.sector_:create_object_from_type(ID_TREE_OBJECT)
			object.name = string.format("Tree %04d", self.editor_.toolbox_.created_objects_count_)	
			self:spawn_object(object)
		end
	end)
	
	root.connect(self.tree_button_.on_up, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then		
			self.editor_:create_open_file_dialog("Open Tree", { "glb" }, function(path)
				self.editor_.selection_.selected_objects_[1]:load_mesh(path)
				self.editor_.selection_:rebuild_selection_bbox()
			end)
		end
	end)
end

function editor_toolbox_objects:init_particle_emitter_button()

	self.particle_emitter_button_ = gwen.Button(self.misc_category_)
	self.particle_emitter_button_:set_bounds(78, 24, 32, 32)
	self.particle_emitter_button_:set_tooltip("Particle Emitter")
	
	root.connect(self.particle_emitter_button_.on_down, function(control)
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if not active_tab.is_planet_ then
		
			local object = active_tab.sector_:create_object_from_type(ID_PARTICLE_EMITTER)
			object.name = string.format("Particle Emitter %04d", self.editor_.toolbox_.created_objects_count_)	
			self:spawn_object(object)
		end
	end)
	
	root.connect(self.particle_emitter_button_.on_up, function(control)
	end)
end