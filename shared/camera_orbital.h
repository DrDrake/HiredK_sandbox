--[[
- @file scene.h
- @brief
]]

class 'camera_orbital' (root.Camera)

function camera_orbital:__init(fov, aspect, near, far, radius)
	root.Camera.__init(self, fov, aspect, vec2(near, far))
    self.radius_ = radius
    
    self.position = vec3(0, 0, radius + (radius * 0.5))
    
    self.target_ = vec3(0, 0, 0)
    self.target_offset_ = vec3(0, 0, 0)
    self:refresh()
    
    self.limit_y_min_ = -math.radians(60.0)
    self.limit_y_max_ = math.radians(60.0)
    self.deg_x_ = 0.0
    self.deg_y_ = 0.0
    
    self.distance_ = math.distance(self.position, self.target_)
    self.max_distance_ = radius + (radius * 1.1)
    self.min_distance_ = radius + 10.0
    self.desired_distance_ = self.distance_
    self.desired_rotation_ = quat()
    self.local_ = mat4()
end

function camera_orbital:get_distance_factor()

    local factor = math.abs(self.desired_distance_ - self.min_distance_) * 0.15
    return math.clamp(factor, 1.0, 150000.0)
end

function camera_orbital:clamp_angle(angle, min, max)

    if angle < -math.radians(360.0) then angle = angle + math.radians(360.0) end
    if angle >  math.radians(360.0) then angle = angle - math.radians(360.0) end   
    return math.clamp(angle, min, max)
end

function camera_orbital:zoom(mouse_wheel_delta)

    self.desired_distance_ = self.desired_distance_ - mouse_wheel_delta * self:get_distance_factor()
    self.desired_distance_ = math.clamp(self.desired_distance_, self.min_distance_, self.max_distance_)
end

function camera_orbital:rotate(mouse_delta)

    self.deg_x_ = self.deg_x_ - (mouse_delta.x * self:get_distance_factor() / self.radius_ * 0.05)
    self.deg_y_ = self.deg_y_ - (mouse_delta.y * self:get_distance_factor() / self.radius_ * 0.05)
    self.deg_y_ = self:clamp_angle(self.deg_y_, self.limit_y_min_, self.limit_y_max_)
end

function camera_orbital:__update(dt)

	local rx = math.angle_axis(self.deg_y_, vec3(math.column(self.local_, 0)))
	local ry = math.angle_axis(self.deg_x_, vec3(0, 1, 0))  
    self.desired_rotation_ = rx * ry

    self.rotation = math.slerp(self.rotation, self.desired_rotation_, dt * 5.0)
    self.distance_ = math.mix(self.distance_, self.desired_distance_, dt * 5.0)  
    self.position = self.target_ - (self.rotation * vec3(0, 0, -1) * self.distance_ + self.target_offset_)
    self:refresh()
    
    self.local_ = math.mat4_cast(self.rotation)
end