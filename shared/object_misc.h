--[[
- @file object_misc.h
- @brief
]]

--[[
////////////////////////////////////////////////////////////////////////////////
// model_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'model_object' (object_base)

function model_object:__init(planet)
    object_base.__init(self, planet)
	self.type_id_ = ID_MODEL_OBJECT
	self.rigid_body_support_ = true
	self.path_ = ""
end

function model_object:load_mesh(path)

	self.mesh_ = FileSystem:search(path, true)
	self.transform_.box:set(self.mesh_.box)
	self.path_ = path
end

function model_object:parse(object)

    self:load_mesh(object:get_string("path"))
	object_base.parse(self, object)
end

function model_object:write(object)

	object:set_string("path", self.path_)
	object_base.write(self, object)
end

function model_object:on_physics_start(dynamic_world)

	if self.rigid_body_enabled_ and self.rigid_body_ == nil then
		self.rigid_body_ = bt.RigidBody.create_collider(dynamic_world, self.transform_, self.mesh_, self.rigid_body_mass_)
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// tree_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'tree_object' (object_base)

function tree_object:__init(planet)
    object_base.__init(self, planet)
	self.type_id_ = ID_TREE_OBJECT
	self.rigid_body_support_ = false
	
	self.shader_ = FileSystem:search("shaders/gbuffer_tree.glsl", true)
	self.animation_scale_ = 0.01
	self.timer_ = 0.0
	self.path_ = ""
end

function tree_object:load_mesh(path)

	self.mesh_ = FileSystem:search(path, true)
	self.transform_.box:set(self.mesh_.box)
	self.path_ = path
end

function tree_object:parse(object)

    self:load_mesh(object:get_string("path"))
	object_base.parse(self, object)
end

function tree_object:write(object)

	object:set_string("path", self.path_)
	object_base.write(self, object)
end

function tree_object:__update(dt)

    self.timer_ = self.timer_ + dt * self.animation_scale_
    object_base.__update(self, dt)
end

function tree_object:draw_shadow(matrix)

	self.shader_:bind()
    root.Shader.get():uniform("u_Timer", self.timer_)
	object_base.draw_shadow(self, matrix)
	root.Shader.pop_stack()
end

function tree_object:draw(camera)

	self.shader_:bind()
	root.Shader.get():uniform("u_Timer", self.timer_)
	object_base.draw(self, camera)
	root.Shader.pop_stack()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// particle_emitter implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'particle_emitter' (object_base)

function particle_emitter:__init(planet)
    object_base.__init(self, planet)
	self.type_id_ = ID_PARTICLE_EMITTER
	self.rigid_body_support_ = false
	
	self.update_shader_ = FileSystem:search("shaders/particle_update.glsl", true)
	self.shader_ = FileSystem:search("shaders/gbuffer_billboard.glsl", true)
	self.albedo_tex_ = root.Texture.from_image(FileSystem:search("images/firework.png", true), { filter_mode = gl.LINEAR })
	self.random_tex_ = root.Texture.from_random_1D(1000)
	self.emitter_ = root.ParticleEmitter(1000)
	self.timer_ = 0.0
end

function particle_emitter:__update(dt)

	self.update_shader_:bind()
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("s_RandomTex", 0)
	self.random_tex_:bind()
	
	root.Shader.get():uniform("u_Delta", dt)
	root.Shader.get():uniform("u_Timer", self.timer_)	
	root.Shader.get():uniform("u_LauncherLifetime", 5.0)
	root.Shader.get():uniform("u_ShellLifetime", 5.0)
	root.Shader.get():uniform("u_SecondaryShellLifetime", 2.5)
	
	self.emitter_:copy(self.transform_)
	self.emitter_:simulate(dt)
	root.Shader.pop_stack()

	self.timer_ = self.timer_ + dt
	object_base.__update(self, dt)
end

function particle_emitter:draw_shadow(matrix)
end

function particle_emitter:draw_transparent(camera)

	self.shader_:bind()
	
	gl.ActiveTexture(gl.TEXTURE0)
	root.Shader.get():sampler("s_AlbedoTex", 0)
	self.albedo_tex_:bind()

	root.Shader.get():uniform("u_CameraPosition", camera.position)
	root.Shader.get():uniform("u_BillboardSize", 0.5)

	self.emitter_:draw(camera)
	root.Shader.pop_stack()
end