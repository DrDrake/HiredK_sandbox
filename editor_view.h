--[[
- @file editor_view.h
- @brief
]]

--[[
////////////////////////////////////////////////////////////////////////////////
// editor_view_base implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'editor_view_base' (gwen.Base)

function editor_view_base:__init(editor, parent)
	gwen.Base.__init(self, parent)
	self:enable_override_render_call()
	self.editor_ = editor
	
	self.gbuffer_shader_ = FileSystem:search("shaders/gbuffer.glsl", true)
	self.render_pipeline_ = render_pipeline(editor.planet_)
	self.camera_ = root.Camera()
	self.last_mouse_pos_ = vec2()
	self.current_dt_ = 0
end

function editor_view_base:__refresh()
    self.render_pipeline_:rebuild(self.editor_.w, self.editor_.h)
end

function editor_view_base:__process_event(event)
end

function editor_view_base:unproject()

	local mouse_pos = self:canvas_pos_to_local(gwen.Point(
		sf.Mouse.get_position(self.editor_).x,
		sf.Mouse.get_position(self.editor_).y
	))
	
	if mouse_pos.x > 0 and mouse_pos.x < self.w and
	   mouse_pos.y > 0 and mouse_pos.y < self.h then
	   
		local scaled_mouse_x = mouse_pos.x * (self.editor_.w / self.w)
		local scaled_mouse_y = mouse_pos.y * (self.editor_.h / self.h)
		return self.render_pipeline_:unproject(self.camera_, scaled_mouse_x, self.editor_.h - scaled_mouse_y)
	end
	
	return vec3()
end

function editor_view_base:__process(dt)
	
	if self.editor_.toolbox_.objects_tab_.page_.is_dragging_ then
	
		local p = self:unproject()
		if math.length(p) > 0 then
			local object_to_add = self.editor_.toolbox_.objects_tab_.page_.object_to_add_
			object_to_add.transform_.position = p + vec3(0, 0.5, 0)
		end
	end

	self.camera_.aspect = self.w / self.h
	self.current_dt_ = dt
	self:redraw()
end

function editor_view_base:__render(skin)

	local final_tex = self.render_pipeline_:render(self.current_dt_, self.camera_)	
    local rect = gwen.Rect(0, 0, 0, 0)
	
    skin:get_render():translate(rect)
	skin:get_render():bind_fbo()
	
    gl.PushAttrib(gl.VIEWPORT_BIT)
    gl.Enable(gl.SCISSOR_TEST)
    
    gl.Viewport(rect.x, self.editor_.h - (rect.y + self.h), self.w, self.h)
    gl.Scissor(rect.x, self.editor_.h - (rect.y + self.h), self.w, self.h)  
    
    gl.ActiveTexture(gl.TEXTURE0)
    root.Shader.get():uniform("u_RenderingType", 2)
    root.Shader.get():sampler("s_Tex0", 0)
    final_tex:bind()
    
    self.render_pipeline_.screen_quad_:draw()
	
	if self.editor_.menu_strip_.dock_selection_toggle_.toggle_state then
		self.editor_.selection_:draw(self.camera_)
	end
	
    gl.Disable(gl.SCISSOR_TEST)
    gl.PopAttrib()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor_planet_view implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'editor_planet_view' (editor_view_base)

function editor_planet_view:__init(editor, parent)
	editor_view_base.__init(self, editor, parent)
	
	self.camera_ = camera_orbital(math.radians(90), 1.0, 0.1, 15000000.0, editor.planet_.radius_)
end

function editor_planet_view:__refresh()

	self.camera_.radius_ = self.editor_.planet_.radius_
	editor_view_base.__refresh(self)
end

function editor_planet_view:__process_event(event)

    if self:is_hovered() and event.type == sf.Event.MouseWheelMoved then
        self.camera_:zoom(event.mouse_wheel.delta)
    end

	editor_view_base.__process_event(self, event)
end

function editor_planet_view:__process(dt)

    local mouse_x = sf.Mouse.get_position(self.editor_).x
    local mouse_y = sf.Mouse.get_position(self.editor_).y
	
	if self:is_hovered() and sf.Mouse.is_button_pressed(sf.Mouse.Right) then
	
		local delta = vec2()
		delta.x = mouse_x - self.last_mouse_pos_.x
		delta.y = mouse_y - self.last_mouse_pos_.y
		self.camera_:rotate(delta)
    end

	self.last_mouse_pos_ = vec2(mouse_x, mouse_y)
	editor_view_base.__process(self, dt)
end

function editor_planet_view:__render(skin)

	for i = 1, 6 do
		self.editor_.planet_.faces_[i].use_tessellated_patch = false
	end

	self.editor_.planet_.enable_logz_ = true
	editor_view_base.__render(self, skin)
	self.editor_.planet_.enable_logz_ = false
	
	for i = 1, 6 do
		self.editor_.planet_.faces_[i].use_tessellated_patch = true
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor_sector_view_base implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'editor_sector_view_base' (editor_view_base)

function editor_sector_view_base:__init(editor, parent, sector)
	editor_view_base.__init(self, editor, parent)
	
	self.deformation_ = root.Deformation()
	self.sector_ = sector
end

function editor_sector_view_base:__process_event(event)

	if self:is_hovered() then
	
		if self.editor_.menu_strip_.dock_selection_toggle_.toggle_state then
			if event.type == sf.Event.MouseButtonPressed and event.mouse_button.button == sf.Mouse.Left then		
				local mouse_pos = self:canvas_pos_to_local(gwen.Point(event.mouse_button.x, event.mouse_button.y))
				self.editor_.selection_:pick_selection(self.camera_, mouse_pos.x, mouse_pos.y, self.w, self.h)
			end
			
			if event.type == sf.Event.MouseMoved and sf.Mouse.is_button_pressed(sf.Mouse.Left) then		
				local mouse_pos = self:canvas_pos_to_local(gwen.Point(event.mouse_move.x, event.mouse_move.y))
				self.editor_.selection_:drag_selection(self.camera_, mouse_pos.x, mouse_pos.y, self.w, self.h)
			end

			if event.type == sf.Event.MouseButtonReleased and event.mouse_button.button == sf.Mouse.Left then
				self.editor_.selection_:release_selection(self.w, self.h)
			end
		end
		
		if self.editor_.menu_strip_.dock_terrain_edit_toggle_.toggle_state then
			if event.type == sf.Event.MouseMoved then
				self.editor_.planet_.pencil_ = vec4(self:unproject(), self.editor_.planet_.pencil_size_)
				self.editor_.planet_.pencil_color_ = root.Color.Red
			end
			
			if event.type == sf.Event.MouseWheelMoved then
				local size_scaling_factor = (sf.Keyboard.is_key_pressed(sf.Keyboard.LControl) and 10.0 or 1.0)
				self.editor_.planet_.pencil_size_ = self.editor_.planet_.pencil_size_ + (event.mouse_wheel.delta * size_scaling_factor)
				self.editor_.planet_.pencil_size_ = math.clamp(self.editor_.planet_.pencil_size_, 2.0, 5000.0)
				self.editor_.planet_.pencil_.w = self.editor_.planet_.pencil_size_
			end
		end
		
		if self.editor_.menu_strip_.dock_road_edit_toggle_.toggle_state then
			if event.type == sf.Event.MouseButtonPressed and event.mouse_button.button == sf.Mouse.Left then
				local terrain = self.editor_.planet_.faces_[self.sector_.face_]			
				terrain.producer_:edit_road_curve(terrain.root, self:unproject())
			end
		end
	end

	editor_view_base.__process_event(self, event)
end

function editor_sector_view_base:__process(dt)

	if self:is_hovered() and self.editor_.menu_strip_.dock_terrain_edit_toggle_.toggle_state then
		if sf.Mouse.is_button_pressed(sf.Mouse.Left) then
			local height_scale = (sf.Keyboard.is_key_pressed(sf.Keyboard.LControl) and -1.0 or 1.0)
			local terrain = self.editor_.planet_.faces_[self.sector_.face_]
			
			terrain.producer_:edit_residual(terrain.root, self.editor_.planet_.pencil_, height_scale)
		end
	end
	
	local terrain_edit = self.editor_.menu_strip_.dock_terrain_edit_toggle_.toggle_state
	local road_edit = self.editor_.menu_strip_.dock_road_edit_toggle_.toggle_state
	self.editor_.planet_.draw_quadtree_ = terrain_edit or road_edit
	self.editor_.planet_.draw_pencil_ = terrain_edit
	editor_view_base.__process(self, dt)
end

function editor_sector_view_base:__render(skin)

	for i = 1, 6 do
		self.editor_.planet_.faces_[i]:set_deformation(self.deformation_)
		self.editor_.planet_.faces_[i].visible_ = self.sector_.face_ == i
	end

	self.editor_.planet_.spherical_ = false
	editor_view_base.__render(self, skin)
	self.editor_.planet_.spherical_ = true
	
	for i = 1, 6 do
		local old_deformation = self.editor_.planet_.faces_[i].spherical_deformation_
		self.editor_.planet_.faces_[i]:set_deformation(old_deformation)
		self.editor_.planet_.faces_[i].visible_ = true
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor_sector_first_person_view implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'editor_sector_first_person_view' (editor_sector_view_base)

function editor_sector_first_person_view:__init(editor, parent, sector)
	editor_sector_view_base.__init(self, editor, parent, sector)

	self.camera_ = camera_first_person(math.radians(90), 1.0, 0.1, 15000000.0)
	self.camera_.position = sector:get_position()
	self.camera_:refresh()
end

function editor_sector_first_person_view:__process(dt)

    local mouse_x = sf.Mouse.get_position(self.editor_).x
    local mouse_y = sf.Mouse.get_position(self.editor_).y
    
	if self:is_hovered() then
		if sf.Mouse.is_button_pressed(sf.Mouse.Right) then
		
			local delta = vec2()
			delta.x = mouse_x - self.last_mouse_pos_.x
			delta.y = mouse_y - self.last_mouse_pos_.y
			self.camera_:rotate(delta)
		end
		
		local speed = (sf.Keyboard.is_key_pressed(sf.Keyboard.LShift) and 1500.0 or 100.0) * dt
		if sf.Keyboard.is_key_pressed(sf.Keyboard.W) then
			self.camera_:move_forward(speed)
		end
		if sf.Keyboard.is_key_pressed(sf.Keyboard.S) then
			self.camera_:move_backward(speed)
		end
		if sf.Keyboard.is_key_pressed(sf.Keyboard.D) then
			self.camera_:move_right(speed)
		end
		if sf.Keyboard.is_key_pressed(sf.Keyboard.A) then
			self.camera_:move_left(speed)
		end
	end
    
    self.last_mouse_pos_ = vec2(mouse_x, mouse_y)
	editor_sector_view_base.__process(self, dt)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor_sector_ortho_view implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'editor_sector_ortho_view' (editor_sector_view_base)

function editor_sector_ortho_view:__init(editor, parent, sector, axis)
	editor_sector_view_base.__init(self, editor, parent, sector)

	self.camera_ = camera_ortho(0, 0, 0, 0, -1000.0, 1000.0, axis)
	self.camera_.position = sector:get_position()
	self.camera_:refresh()
	
	-- temp camera ortho reset, todo better
	self.camera_.desired_zoom_ = self.camera_.zoom_
	self.camera_.desired_position_ = vec3(self.camera_.position)
end

function editor_sector_ortho_view:__process_event(event)

	if self:is_hovered() and event.type == sf.Event.MouseWheelMoved then
		if not sf.Keyboard.is_key_pressed(sf.Keyboard.LControl) then
			self.camera_:zoom(event.mouse_wheel.delta)
		end
	end

	editor_sector_view_base.__process_event(self, event)
end

function editor_sector_ortho_view:__process(dt)

    local mouse_x = sf.Mouse.get_position(self.editor_).x
    local mouse_y = sf.Mouse.get_position(self.editor_).y
	
	if self:is_hovered() and sf.Mouse.is_button_pressed(sf.Mouse.Right) then
	
		local delta = vec2()
		delta.x = mouse_x - self.last_mouse_pos_.x
		delta.y = mouse_y - self.last_mouse_pos_.y
		self.camera_:drag(delta)
	end
    
    self.camera_.fixed_bounds_.x = -self.w * 0.5
    self.camera_.fixed_bounds_.y =  self.w * 0.5
    self.camera_.fixed_bounds_.z = -self.h * 0.5
    self.camera_.fixed_bounds_.w =  self.h * 0.5  
	
    self.last_mouse_pos_ = vec2(mouse_x, mouse_y)
	editor_sector_view_base.__process(self, dt)
end