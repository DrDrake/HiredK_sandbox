--[[
- @file camera_third_person.h
- @brief
]]

class 'camera_third_person' (root.Camera)

function camera_third_person:__init(fov, aspect, near, far)
	root.Camera.__init(self, fov, aspect, vec2(near, far))
	
	self.distance_ = 8.0
	self.euler_ = vec3()
	self.local_ = mat4()
end

function camera_third_person:rotate(mouse_delta)
	self.euler_.x = self.euler_.x - (mouse_delta.y * 0.001)
    self.euler_.y = self.euler_.y - (mouse_delta.x * 0.001)
end

function camera_third_person:__update(dt)

	self.position.x = self.distance_ * math.cos(self.euler_.x) * math.sin(-self.euler_.y)
	self.position.y = self.distance_ * math.sin(self.euler_.x)
	self.position.z = self.distance_ * math.cos(self.euler_.x) * math.cos(-self.euler_.y)	
	self.rotation = math.conjugate(quat(math.lookat(self.position, vec3(), vec3(0, 1, 0))))
	self:refresh()
	
	self.local_ = math.mat4_cast(self.rotation)
end