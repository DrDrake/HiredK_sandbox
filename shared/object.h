--[[
- @file object.h
- @brief
]]

class 'object' (root.ScriptObject)

function object:__init(scene)
    root.ScriptObject.__init(self, "object")
    self.scene_ = scene
    
    self.shader_ = FileSystem:search("shaders/gbuffer.glsl", true)
    self.picking_shader_ = FileSystem:search("shaders/gbuffer_picking.glsl", true)
    self.transform_ = root.Transform()
    self.meshes_ = {}
    self.body_ = nil
end

function object:parse(config)

    self.transform_.position.x = config:get_number("pos_x")
    self.transform_.position.y = config:get_number("pos_y")
    self.transform_.position.z = config:get_number("pos_z")
    
    self.transform_.scale.x = config:get_number("scale_x")
    self.transform_.scale.y = config:get_number("scale_y")
    self.transform_.scale.z = config:get_number("scale_z")
end

function object:write(config)

    config:set_number("pos_x", self.transform_.position.x)
    config:set_number("pos_y", self.transform_.position.y)
    config:set_number("pos_z", self.transform_.position.z)
    
    config:set_number("scale_x", self.transform_.scale.x)
    config:set_number("scale_y", self.transform_.scale.y)
    config:set_number("scale_z", self.transform_.scale.z)
end

function object:on_physics_start(dynamic_world)
end

function object:draw_shadow(matrix)

    self.shader_:bind()    
    root.Shader.get():uniform("u_ViewProjMatrix", matrix)  
    
    for i = 1, table.getn(self.meshes_) do
        self.meshes_[i]:copy(self.transform_)
        self.meshes_[i]:draw()
    end
    
    root.Shader.pop_stack()
end

function object:draw_unique_color(camera)

    self.picking_shader_:bind()    
    root.Shader.get():uniform("u_EnableColor", 1)
    root.Shader.get():uniform("u_Color", self.unique_color)   
    
    for i = 1, table.getn(self.meshes_) do
        self.meshes_[i]:copy(self.transform_)
        self.meshes_[i]:draw(camera)
    end
    
    root.Shader.get():uniform("u_EnableColor", 0)
    root.Shader.pop_stack()
end