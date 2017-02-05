--[[
- @file camera_ortho.h
- @brief
]]

class 'camera_ortho' (root.Camera)

function camera_ortho:__init(left, right, bottom, top, near, far, axis)
	root.Camera.__init(self, vec4(left, right, bottom, top), vec2(near, far))
    
    self.fixed_bounds_ = vec4(left, right, bottom, top)  
    self.axis_ = axis
    
    self.min_zoom_ = 0.005
    self.max_zoom_ = 10.0
    self.zoom_ = 0.1
    
    self.desired_zoom_ = self.zoom_
    self.desired_position_ = vec3()
end

function camera_ortho:get_zoom_factor()
    return math.max(self.min_zoom_, math.abs(self.min_zoom_ - self.desired_zoom_))
end

function camera_ortho:zoom(mouse_wheel_delta)

    self.desired_zoom_ = self.desired_zoom_ - (mouse_wheel_delta * self:get_zoom_factor() * 0.15)
    self.desired_zoom_ = math.clamp(self.desired_zoom_, self.min_zoom_, self.max_zoom_)
end

function camera_ortho:drag(mouse_delta)

    if self.axis_ == math.NegX then
        self.desired_position_.y = self.desired_position_.y + (mouse_delta.y * self:get_zoom_factor() * 2.5)
        self.desired_position_.z = self.desired_position_.z - (mouse_delta.x * self:get_zoom_factor() * 2.5)
        return
    end
    
    if self.axis_ == math.NegY then
        self.desired_position_.x = self.desired_position_.x - (mouse_delta.x * self:get_zoom_factor() * 2.5)
        self.desired_position_.z = self.desired_position_.z - (mouse_delta.y * self:get_zoom_factor() * 2.5)
        return
    end
    
    if self.axis_ == math.NegZ then
        self.desired_position_.x = self.desired_position_.x + (mouse_delta.x * self:get_zoom_factor() * 2.5)
        self.desired_position_.y = self.desired_position_.y + (mouse_delta.y * self:get_zoom_factor() * 2.5)
        return
    end
end

function camera_ortho:__update(dt)

    self.zoom_ = math.mix(self.zoom_, self.desired_zoom_, dt * 5.0)
    self.position = math.mix(self.position, self.desired_position_, dt * 5.0)
    self.rotation = quat.from_axis(self.axis_)
    self.bounds = self.fixed_bounds_ * self.zoom_
	self:refresh()
end