--[[
- @file object_common.h
- @brief
]]

--[[
////////////////////////////////////////////////////////////////////////////////
// cube_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'cube_object' (object)

function cube_object:__init(scene)
    object.__init(self, scene)
    
    table.insert(self.meshes_, root.Mesh.build_cube())
end

function cube_object:write(config)

    config:set_number("type", 1)  
    object.write(self, config)
end

function cube_object:on_physics_start(dynamic_world)

    self.body_ = bt.RigidBody.create_box(dynamic_world, self.transform_.position, vec3(0.5), 1.0)
end

function cube_object:draw(camera)

    self.shader_:bind()
    root.Shader.get():uniform("u_EnableColor", 1)
    root.Shader.get():uniform("u_Color", root.Color.White)
    
    if self.body_ ~= nil then
        self.transform_.position = self.body_.position
        self.transform_.rotation = self.body_.rotation
    end
    
    self.meshes_[1]:copy(self.transform_)
    self.meshes_[1]:draw(camera)
    
    root.Shader.get():uniform("u_EnableColor", 0)
    root.Shader.pop_stack()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// cubemap_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'cubemap_object' (object)

function cubemap_object:__init(scene, size)
    object.__init(self, scene)
    self.size_ = size
    
    self.sky_shader_ = FileSystem:search("shaders/sky.glsl", true)    
    self.cubemap_camera_ = root.Camera(math.radians(90.0), 1.0, vec2(1.0, 15000.0))
    self.screen_quad_ = root.Mesh.build_quad()
    
    self.cubemap_fbo_ = root.FrameBuffer()
    self.cubemap_tex_ = root.Texture.from_empty(size, size, { target = gl.TEXTURE_CUBE_MAP, iformat = gl.RGBA16F, type = gl.FLOAT, filter_mode = gl.LINEAR })   
    self.clouds_tex_ = root.Texture.from_empty(size, size, { iformat = gl.RGBA8, filter_mode = gl.LINEAR })
    
    self.current_face_ = 1
end

function cubemap_object:write(config)

    config:set_string("size", self.size_)
    config:set_number("type", 2)  
    object.write(self, config)
end

function cubemap_object:generate()

    gl.PushAttrib(gl.VIEWPORT_BIT)
    gl.Viewport(0, 0, self.size_, self.size_)
    
    for i = 1, 6 do
    
        if i == self.current_face_ and (i - 1) ~= math.NegY then
        
            self.cubemap_fbo_:clear()
            self.cubemap_fbo_:attach(self.clouds_tex_, gl.COLOR_ATTACHMENT0)
            self.cubemap_fbo_:bind_output()
            
            self.cubemap_camera_.position = vec3()
            self.cubemap_camera_.rotation = quat.from_axis_cubemap(i - 1)
            self.cubemap_camera_:refresh()
            
            gl.ClearColor(root.Color.Transparent)
            gl.Clear(gl.COLOR_BUFFER_BIT)
            
            self.scene_.planet_.clouds_:draw(self.cubemap_camera_, 8)
        
            self.cubemap_fbo_:clear()
            self.cubemap_fbo_:attach(self.cubemap_tex_, gl.COLOR_ATTACHMENT0, i - 1)
            self.cubemap_fbo_:bind_output()
            
            self.cubemap_camera_.position = vec3(0.0, 6360000.0 + 1500.0, 0.0)
            self.cubemap_camera_:refresh()

            gl.ClearColor(root.Color.Black)
            gl.Clear(gl.COLOR_BUFFER_BIT)
            
            self.sky_shader_:bind()
            root.Shader.get():uniform("u_InvProjMatrix", math.inverse(self.cubemap_camera_.proj_matrix))
            root.Shader.get():uniform("u_InvViewMatrix", math.inverse(self.cubemap_camera_.view_matrix))         
            root.Shader.get():uniform("u_CameraPos", self.cubemap_camera_.position)
            root.Shader.get():uniform("u_SunDir", self.scene_.sun_:get_direction())   
            self.scene_.planet_.atmosphere_:bind(1000.0)
            
            gl.ActiveTexture(gl.TEXTURE0)
            root.Shader.get():sampler("s_Clouds", 0)
            self.clouds_tex_:bind()
            
            self.screen_quad_:draw()       
            root.Shader.pop_stack()
            break
        end
    end
    
    self.current_face_ = self.current_face_ + 1
    if self.current_face_ > 6 then
        self.current_face_ = 1
    end
    
    --self.cubemap_tex_:generate_mipmap()  
	root.FrameBuffer.unbind()
	gl.PopAttrib()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// model_object implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'model_object' (object)

function model_object:__init(scene, path)
    object.__init(self, scene)
    self.path_ = path
    
    table.insert(self.meshes_, FileSystem:search(path, true))
    self.transform_.box:set(self.meshes_[1].box)
end

function model_object:draw(camera)

    self.shader_:bind()
    self.meshes_[1]:copy(self.transform_)
    self.meshes_[1]:draw(camera)
    root.Shader.pop_stack()
end

function model_object:write(config)

    config:set_string("path", self.path_)
    config:set_number("type", 3)
    object.write(self, config)
end