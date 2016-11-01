--[[
- @file first_person_camera.h
- @brief
]]

class 'first_person_camera' (root.Camera)

function first_person_camera:__init(fov, aspect, near, far)
	root.Camera.__init(self, fov, aspect, vec2(near, far))
	
	self.prev_view_proj_origin_matrix_ = mat4()
	self.velocity_damp_ = 0.00001
	self.velocity_ = vec3()
	self.rotation_damp_ = 0.000005
	self.euler_ = vec3()
	
	self.mouse_pos_ = vec2()
	self.local_ = mat4()
end

function first_person_camera:move_forward(factor)
	self.velocity_ = self.velocity_ - vec3(math.column(self.local_, 2)) * factor
end

function first_person_camera:move_backward(factor)
	self:move_forward(-factor)
end

function first_person_camera:move_right(factor)
	self.velocity_ = self.velocity_ + vec3(math.column(self.local_, 0)) * factor
end

function first_person_camera:move_left(factor)
	self:move_right(-factor)
end

function first_person_camera:rotate_x(factor)
	self.euler_.x = self.euler_.x + factor
end

function first_person_camera:rotate_y(factor)
	self.euler_.y = self.euler_.y + factor
end

function first_person_camera:__update(dt)

	local mouse_x = sf.Mouse.get_position().x
	local mouse_y = sf.Mouse.get_position().y
	
	if sf.Mouse.is_button_pressed(sf.Mouse.Right) then
		self:rotate_x((self.mouse_pos_.y - mouse_y) * 0.001)
		self:rotate_y((self.mouse_pos_.x - mouse_x) * 0.001)
	end
	
	local speed = (sf.Keyboard.is_key_pressed(sf.Keyboard.LShift) and 100.0 or 15.0) * dt	
	if sf.Keyboard.is_key_pressed(sf.Keyboard.E) then
		speed = 10000.0 * dt
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.F) then
		speed = 1000000.0 * dt
	end
	
	if sf.Keyboard.is_key_pressed(sf.Keyboard.W) then
		self:move_forward(speed)
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.S) then
		self:move_backward(speed)
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.D) then
		self:move_right(speed)
	end
	if sf.Keyboard.is_key_pressed(sf.Keyboard.A) then
		self:move_left(speed)
	end
	
	local rx = math.angle_axis(math.degrees(self.euler_.x * dt), vec3(math.column(self.local_, 0)))
	local ry = math.angle_axis(math.degrees(self.euler_.y * dt), vec3(0, 1, 0));
	local rz = math.angle_axis(math.degrees(self.euler_.z * dt), vec3(math.column(self.local_, 2)))
	
	self.position = self.position + (self.velocity_ * dt)
	self.rotation = math.normalize(rz*ry*rx * self.rotation)
	
	self.prev_view_proj_origin_matrix_ = mat4(self.view_proj_origin_matrix)
	self:refresh()
	
	self.velocity_ = self.velocity_ * math.pow(self.velocity_damp_, dt)
	self.euler_ = self.euler_ * math.pow(self.rotation_damp_, dt)	
	self.local_ = math.translate(mat4(), self.position) * math.mat4_cast(self.rotation)
	self.mouse_pos_ = vec2(mouse_x, mouse_y)
end