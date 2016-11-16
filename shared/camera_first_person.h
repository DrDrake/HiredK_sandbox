--[[
- @file camera_first_person.h
- @brief
]]

class 'camera_first_person' (root.Camera)

function camera_first_person:__init(fov, aspect, near, far)
	root.Camera.__init(self, fov, aspect, vec2(near, far))
	
	self.velocity_damp_ = 0.00001
	self.velocity_ = vec3()
	self.rotation_damp_ = 0.000005
	self.euler_ = vec3()
	self.local_ = mat4()
end

function camera_first_person:move_forward(factor)
	self.velocity_ = self.velocity_ - vec3(math.column(self.local_, 2)) * factor
end

function camera_first_person:move_backward(factor)
	self:move_forward(-factor)
end

function camera_first_person:move_right(factor)
	self.velocity_ = self.velocity_ + vec3(math.column(self.local_, 0)) * factor
end

function camera_first_person:move_left(factor)
	self:move_right(-factor)
end

function camera_first_person:rotate(mouse_delta)
	self.euler_.x = self.euler_.x - (mouse_delta.y * 0.001)
    self.euler_.y = self.euler_.y - (mouse_delta.x * 0.001)
end

function camera_first_person:__update(dt)

	local rx = math.angle_axis(math.degrees(self.euler_.x * dt), vec3(math.column(self.local_, 0)))
	local ry = math.angle_axis(math.degrees(self.euler_.y * dt), vec3(0, 1, 0));
	local rz = math.angle_axis(math.degrees(self.euler_.z * dt), vec3(math.column(self.local_, 2)))
	
	self.position = self.position + (self.velocity_ * dt)
	self.rotation = math.normalize(rz*ry*rx * self.rotation)
	self:refresh()
	
	self.velocity_ = self.velocity_ * math.pow(self.velocity_damp_, dt)
	self.euler_ = self.euler_ * math.pow(self.rotation_damp_, dt)	
	self.local_ = math.translate(mat4(), self.position) * math.mat4_cast(self.rotation)
end