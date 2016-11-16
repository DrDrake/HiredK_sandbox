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
    
    self.sector_camera_ = root.Camera()
    self.planet_camera_ = root.Camera()
    self.render_pipeline_ = render_pipeline(editor.scene_)
    self.render_pipeline_.enable_shadow_ = false
    self.render_pipeline_.enable_occlusion_ = false
    self.last_mouse_pos_ = vec2()
end

function editor_view_base:refresh_gui(w, h)

    self.render_pipeline_:rebuild(w, h)
end

function editor_view_base:__render(skin)

    self.planet_camera_.aspect = self.w/self.h
    self.sector_camera_.aspect = self.w/self.h
    
    local final_tex = self.render_pipeline_:render(self.planet_camera_, self.sector_camera_)  
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
    
    local radius =  self.editor_.scene_.planet_.radius_
    self.planet_camera_ = camera_orbital(math.radians(90), 1.0, 0.1, 15000000.0, radius)
    self.sector_camera_ = root.Camera()
end

function editor_planet_view:refresh_gui(w, h)

    local radius =  self.editor_.scene_.planet_.radius_
    self.planet_camera_.radius_ = radius
    
    editor_view_base.refresh_gui(self, w, h)
end

function editor_planet_view:process_event(event)

    if event.type == sf.Event.MouseWheelMoved then
       self.planet_camera_:zoom(event.mouse_wheel.delta)
    end
end

function editor_planet_view:process(dt)

    local mouse_x = sf.Mouse.get_position(self.editor_).x
    local mouse_y = sf.Mouse.get_position(self.editor_).y
    
    if sf.Mouse.is_button_pressed(sf.Mouse.Right) then
    
        local delta = vec2()
        delta.x = mouse_x - self.last_mouse_pos_.x
        delta.y = mouse_y - self.last_mouse_pos_.y
        self.planet_camera_:rotate(delta)
    end
    
    self.last_mouse_pos_ = vec2(mouse_x, mouse_y)
    self:redraw()
end

function editor_planet_view:__render(skin)

	self.sector_camera_:copy(self.planet_camera_)
    self.sector_camera_.position = self.planet_camera_.target_ - (self.planet_camera_.rotation * vec3(0, 0, -1) * self.planet_camera_.distance_ + vec3(0, 0, 6360000.0))
	self.sector_camera_:refresh()
    
    editor_view_base.__render(self, skin)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor_sector_view implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'editor_sector_view' (editor_view_base)

function editor_sector_view:__init(editor, parent, sector)
	editor_view_base.__init(self, editor, parent)
    self.sector_ = sector

    self.sector_camera_ = camera_first_person(math.radians(90), 1.0, 0.1, 15000000.0)
    self.planet_camera_ = root.Camera()
end

function editor_sector_view:process(dt)
    
    local mouse_x = sf.Mouse.get_position(self.editor_).x
    local mouse_y = sf.Mouse.get_position(self.editor_).y
    
    if sf.Mouse.is_button_pressed(sf.Mouse.Right) then
    
        local delta = vec2()
        delta.x = mouse_x - self.last_mouse_pos_.x
        delta.y = mouse_y - self.last_mouse_pos_.y
        self.sector_camera_:rotate(delta)
    end
    
	local speed = (sf.Keyboard.is_key_pressed(sf.Keyboard.LShift) and 15000.0 or 100.0) * dt
	if sf.Keyboard.is_key_pressed(sf.Keyboard.W) then
		self.sector_camera_:move_forward(speed)
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.S) then
		self.sector_camera_:move_backward(speed)
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.D) then
		self.sector_camera_:move_right(speed)
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.A) then
		self.sector_camera_:move_left(speed)
	end
    
    self.last_mouse_pos_ = vec2(mouse_x, mouse_y)
    self:redraw()
end

function editor_sector_view:__render(skin)

	self.planet_camera_:copy(self.sector_camera_)
    self.planet_camera_.position = self.sector_camera_.position + self.sector_.position_
	self.planet_camera_:refresh()

    editor_view_base.__render(self, skin)
end