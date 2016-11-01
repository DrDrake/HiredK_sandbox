--[[
- @file scene.h
- @brief
]]

#include "planet.h"
#include "sun.h"

class 'scene' (root.ScriptObject)

function scene:__init()
    root.ScriptObject.__init(self, "scene")	
	self.type = root.ScriptObject.Dynamic
	
	self.sun_ = sun(60.0, 20.0, 4096.0, 4)
	self.objects_ = {}
	
	self.planet_ = planet(self, 6360000.0)
	self.planet_.shadow_ = false
	table.insert(self.objects_, self.planet_)
	
	self.lucy_ = FileSystem:search("models/lucy.glb", true)
	self.lucy_.shader_ = FileSystem:search("shaders/gbuffer.glsl", true)
	self.lucy_.shadow_ = true
	self.lucy_.position = vec3(-585.68787505049, -2.0, -82.940718697932)
	self.lucy_.scale = vec3(50.0)
	table.insert(self.objects_, self.lucy_)
end

function scene:__update(dt)
end

function scene:make_shadow(camera)

	gl.Enable(gl.DEPTH_TEST)

	self.sun_:make_sun_shadow(camera, function(matrix)
		for i = 1, table.getn(self.objects_) do
			if self.objects_[i].shadow_ then
				self.objects_[i].shader_:bind()
				root.Shader.get():uniform("u_ViewProjMatrix", matrix)
				self.objects_[i]:draw()
				root.Shader.pop_stack()
			end
		end
	end)
	
	gl.Disable(gl.DEPTH_TEST)
end

function scene:draw(camera)

	if sf.Keyboard.is_key_pressed(sf.Keyboard.Space) then
		gl.PolygonMode(gl.FRONT_AND_BACK, gl.LINE)
	end
	
    gl.Enable(gl.DEPTH_TEST)
    gl.Enable(gl.CULL_FACE)

	for i = 1, table.getn(self.objects_) do
		self.objects_[i].shader_:bind()
		self.objects_[i]:draw(camera)
		root.Shader.pop_stack()
	end
	
	gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
	gl.Disable(gl.DEPTH_TEST)
    gl.Disable(gl.CULL_FACE)
end