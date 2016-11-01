--[[
- @file planet.h
- @brief
]]

#include "planet_atmosphere.h"
#include "planet_ocean.h"

class 'planet' (root.ScriptObject)

function planet:__init(scene, radius)
    root.ScriptObject.__init(self, "planet")
	self.scene_ = scene
	self.radius_ = radius
	
	self.shader_ = FileSystem:search("shaders/gbuffer_planet.glsl", true)
	self.local_camera_ = root.Camera()
	self.faces_ = {}
	
	local height_shader = FileSystem:search("shaders/height.glsl", true)
	local normal_shader = FileSystem:search("shaders/normal.glsl", true)
	
	for i = 1, 6 do
		self.faces_[i] = root.Terrain(radius, i - 1)
		self.faces_[i]:use_height_normal_producer(height_shader, normal_shader, 123213)
		self.faces_[i]:use_spherical_deformation()
	end
	
	self.atmosphere_ = planet_atmosphere(self)
	self.atmosphere_:generate()
	
	self.ocean_shader_ = FileSystem:search("shaders/gbuffer_ocean.glsl", true)
	self.ocean_ = planet_ocean(self)
end

function planet:draw(camera)

	self.local_camera_:copy(camera)	
	self.local_camera_.position.y = camera.position.y + self.radius_ + 10.0
	self.local_camera_:refresh()
	
	self.ocean_shader_:bind()
	self.ocean_:draw(self.local_camera_)
	root.Shader.pop_stack()
	
	root.Shader.get():uniform("u_OceanLevel", 1.0)
	root.Shader.get():sampler("u_DrawOcean", not (self.ocean_.draw_ocean_) and 1 or 0)
	self.atmosphere_:bind(1000.0)
		
	for i = 1, 6 do
		self.faces_[i]:draw(self.local_camera_)
	end
end