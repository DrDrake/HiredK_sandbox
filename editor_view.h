--[[
- @file editor_view.h
- @brief
]]

--[[
////////////////////////////////////////////////////////////////////////////////
// editor_view_base implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'editor_view_base' (gwen.Base)

function editor_view_base:__init(editor, parent)
	gwen.Base.__init(self, parent)
	self.editor_ = editor
    
    self.render_pipeline_ = render_pipeline(editor.scene_)
    self.sector_camera_ = root.Camera()
    self.planet_camera_ = root.Camera()
end

function editor_view_base:refresh_gui(w, h)

    self.render_pipeline_:rebuild(w, h)
end

function editor_view_base:render_scene(skin, draw_func)

    self.planet_camera_.aspect = self.w/self.h
    self.sector_camera_.aspect = self.w/self.h 

    local final_tex = self.render_pipeline_:render(self.planet_camera_, self.sector_camera_, draw_func)
    local rect = gwen.Rect(0, 0, 0, 0)
    skin:get_render():translate(rect)
    
    skin:get_render():bind_fbo()
    
    gl.PushAttrib(gl.VIEWPORT_BIT)
    gl.Enable(gl.SCISSOR_TEST)
    
    gl.Viewport(rect.x, self.editor_.h - (rect.y + self.h), self.w, self.h)
    gl.Scissor(rect.x, self.editor_.h - (rect.y + self.h), self.w, self.h)  
    
    gl.ActiveTexture(gl.TEXTURE0)
    root.Shader.get():uniform("u_RenderingType", 2)
    root.Shader.get():sampler("s_Tex0", 0)
    final_tex:bind()
    
    self.render_pipeline_.screen_quad_:draw()
end

function editor_view_base:end_render()

    gl.Disable(gl.SCISSOR_TEST)
    gl.PopAttrib()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor_planet_view implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'editor_planet_view' (editor_view_base)

function editor_planet_view:__init(editor, parent)
	editor_view_base.__init(self, editor, parent)
    self:enable_override_render_call()
    
    local radius =  self.editor_.scene_.planet_.radius_
    self.planet_camera_ = camera_orbital(math.radians(90), 1.0, 0.1, 15000000.0, radius)
    self.sector_camera_ = root.Camera()
    self.last_mouse_pos_ = vec2()

    self.render_pipeline_.enable_shadow_ = false
    self.render_pipeline_.enable_occlusion_ = false
    self.render_pipeline_.enable_reflection_ = false
    self.planet_camera_.enable_logz_ = true
end

function editor_planet_view:refresh_gui(w, h)

    local radius =  self.editor_.scene_.planet_.radius_
    self.planet_camera_.radius_ = radius
    
    editor_view_base.refresh_gui(self, w, h)
end

function editor_planet_view:process_event(event)

    if self:is_hovered() then
        if event.type == sf.Event.MouseWheelMoved then
           self.planet_camera_:zoom(event.mouse_wheel.delta)
        end
    end
end

function editor_planet_view:process(dt)

    local mouse_x = sf.Mouse.get_position(self.editor_).x
    local mouse_y = sf.Mouse.get_position(self.editor_).y
    
    if self:is_hovered() then
        if sf.Mouse.is_button_pressed(sf.Mouse.Right) then
        
            local delta = vec2()
            delta.x = mouse_x - self.last_mouse_pos_.x
            delta.y = mouse_y - self.last_mouse_pos_.y
            self.planet_camera_:rotate(delta)
        end
    end
    
    self.last_mouse_pos_ = vec2(mouse_x, mouse_y)
    self:redraw()
end

function editor_planet_view:__render(skin)

    if self.editor_.scene_:is_planet_loaded() then
        self.sector_camera_:copy(self.planet_camera_)
        self.sector_camera_.position = self.planet_camera_.target_ - (self.planet_camera_.rotation * vec3(0, 0, -1) * self.planet_camera_.distance_ + vec3(0, 0, 6360000.0))
        self.sector_camera_:refresh()
        
        self:render_scene(skin, function(planet_camera, sector_camera)
            self.editor_.scene_:draw(planet_camera, sector_camera, true)
        end)
        
        self:end_render()
    end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor_sector_view_base implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'editor_sector_view_base' (editor_view_base)

function editor_sector_view_base:__init(editor, parent, sector)
	editor_view_base.__init(self, editor, parent)
    self:enable_override_render_call()
    self.sector_ = sector
    
    self.terrain_shader_ = FileSystem:search("shaders/gbuffer_terrain.glsl", true)
    self.deformation_ = root.Deformation()
    
    self.grid_shader_ = FileSystem:search("shaders/gbuffer.glsl", true)
    self.grid_tex_ = root.Texture.from_image(FileSystem:search("grid.png", true), { filter_mode = gl.LINEAR_MIPMAP_LINEAR, wrap_mode = gl.REPEAT, mipmap = true, anisotropy = 16.0 })  
    self.grid_mesh_ = root.Mesh.build_plane()
    
    self.selection_shader_ = FileSystem:search("shaders/gbuffer_picking.glsl", true)
    self.selection_quad_ = root.Mesh.build_quad(1, 1, false)
    self.selection_mouse_pos_ = gwen.Point()
    self.selection_bbox_ = Box3D()
    self.selection_visible_ = false
    
    self.manipulator_shader_ = FileSystem:search("shaders/gbuffer_picking.glsl", true)
    self.cone_shape_ = FileSystem:search("cone.glb", true)
    
    self.manipulator_axis_ = {}
    self.manipulator_axis_[1] = root.Transform() -- x
    self.manipulator_axis_[2] = root.Transform() -- y
    self.manipulator_axis_[3] = root.Transform() -- z
    self.manipulator_center_ = vec3()
    
    self.drag_plane_ = root.Mesh.build_plane()
    self.drag_position_ = vec3()
    self.is_dragging_ = false
end

function editor_sector_view_base:refresh_gui(w, h)

    self.selection_fbo_ = root.FrameBuffer()
    self.selection_depth_tex_ = root.Texture.from_empty(w, h, { iformat = gl.DEPTH_COMPONENT32F, format = gl.DEPTH_COMPONENT, type = gl.FLOAT })
    self.selection_color_tex_ = root.Texture.from_empty(w, h)
    editor_view_base.refresh_gui(self, w, h)
end

function editor_sector_view_base:project_mouse(x, y)

    local depth = self.selection_fbo_:read_pixel(self.selection_depth_tex_, gl.DEPTH_COMPONENT, x, y).x   
    local pos = math.unproject(vec3(x, y, depth), self.sector_camera_.view_matrix, self.sector_camera_.proj_matrix, vec4(0, 0, self.w, self.h))
    return pos
end

function editor_sector_view_base:compute_drag_position(x, y)

    self.selection_fbo_:clear()
    self.selection_fbo_:attach(self.selection_depth_tex_, gl.DEPTH_ATTACHMENT)
    self.selection_fbo_:bind_output()
    
    gl.Enable(gl.DEPTH_TEST)
    gl.Viewport(0, 0, self.w, self.h)
    gl.Clear(gl.DEPTH_BUFFER_BIT)
    
    if self.editor_.tab_control_.current_axis_ == 2 then
        self.drag_plane_.rotation = math.angle_axis(math.radians(90), vec3(1, 0, 0))
    else
        self.drag_plane_.rotation = quat()
    end
    
    self.manipulator_shader_:bind()
    self.drag_plane_:draw(self.sector_camera_)
    
    if self.editor_.tab_control_.current_axis_ == 2 then
        self.drag_plane_.rotation = math.angle_axis(math.radians(90), vec3(0, 0, 1))
        self.drag_plane_:draw(self.sector_camera_)
    end
    
    root.Shader.pop_stack()   
    return self:project_mouse(x, y)
end

function editor_sector_view_base:process_event(event)

    if self:is_hovered() then
    
        if event.type == sf.Event.MouseButtonPressed and event.mouse_button.button == sf.Mouse.Left then  
        
            local pos = self:canvas_pos_to_local(gwen.Point(event.mouse_button.x, event.mouse_button.y))
            
            self.selection_quad_.position.x = (pos.x * (1.0 / self.w)) * 2.0 - 1.0
            self.selection_quad_.position.y = (1.0 - (pos.y * (1.0 / self.h))) * 2.0 - 1.0
            self.selection_quad_.scale.x = 0.0
            self.selection_quad_.scale.y = 0.0
            self.selection_mouse_pos_ = pos
            self.selection_visible_ = true
            
            self.selection_fbo_:clear()
            self.selection_fbo_:attach(self.selection_depth_tex_, gl.DEPTH_ATTACHMENT)
            self.selection_fbo_:attach(self.selection_color_tex_, gl.COLOR_ATTACHMENT0)
            self.selection_fbo_:bind_output()
            
            gl.ClearColor(root.Color.Black)
            gl.Clear(bit.bor(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT))
            gl.Viewport(0, 0, self.w, self.h)          
            gl.Enable(gl.DEPTH_TEST)
            
            for i = 1, table.getn(self.sector_.objects_) do
                self.sector_.objects_[i]:draw_unique_color(self.sector_camera_)
            end
            
            gl.Clear(gl.DEPTH_BUFFER_BIT)
            self:draw_manipulator(self.sector_camera_, 0.6, true)
            gl.Disable(gl.DEPTH_TEST)
            
            local pixel = self.selection_fbo_:read_color(self.selection_color_tex_, gl.COLOR_ATTACHMENT0, pos.x, self.h - pos.y)
            for i = 1, table.getn(self.manipulator_axis_) do          
                if pixel:to_integer() == self.manipulator_axis_[i].unique_color:to_integer() then
                
                    self.drag_plane_.position = self.manipulator_axis_[i].position
                    self.drag_plane_.scale = vec3(4096.0)
                    self.drag_position_ = self:compute_drag_position(pos.x, self.h - pos.y)
                    self.editor_.tab_control_.current_axis_ = i
                    self.is_dragging_ = true
                    break
                end
            end
            
            if not self.is_dragging_ then
            
                self.editor_.tab_control_.selection_ = {}
                
                for i = 1, table.getn(self.sector_.objects_) do
                    if pixel:to_integer() == self.sector_.objects_[i].unique_color:to_integer() then
                        table.insert(self.editor_.tab_control_.selection_, self.sector_.objects_[i])
                        self.editor_.tab_control_.current_axis_ = 0
                        break
                    end
                end
            end
        end
        
        if event.type == sf.Event.MouseMoved and sf.Mouse.is_button_pressed(sf.Mouse.Left) then
        
            local pos = self:canvas_pos_to_local(gwen.Point(event.mouse_move.x, event.mouse_move.y))       
            if self.is_dragging_ then
            
                local new_drag_position = self:compute_drag_position(pos.x, self.h - pos.y)
                local delta = self.drag_position_ - new_drag_position
                
                for i = 1, table.getn(self.editor_.tab_control_.selection_) do
                    local transform = self.editor_.tab_control_.selection_[i].transform_
                    if self.editor_.tab_control_.current_axis_ == 1 then
                        transform.position.x = transform.position.x - delta.x
                    end
                    if self.editor_.tab_control_.current_axis_ == 2 then
                        transform.position.y = transform.position.y - delta.y
                    end
                    if self.editor_.tab_control_.current_axis_ == 3 then
                        transform.position.z = transform.position.z - delta.z
                    end
                end
                
                self.drag_position_ = new_drag_position
                
            else          
                self.selection_quad_.scale.x = (pos.x - self.selection_mouse_pos_.x) * (1.0 / self.w * 2.0)
                self.selection_quad_.scale.y = -(pos.y - self.selection_mouse_pos_.y) * (1.0 / self.h * 2.0)
            end
        end
        
        root.FrameBuffer.unbind()
    end
    
    if event.type == sf.Event.MouseButtonReleased then  
    
        if not self.is_dragging_ and event.mouse_button.button == sf.Mouse.Left and self:is_hovered() then

            local w = math.floor(self.selection_quad_.scale.x * self.w * 0.5)
            local h = math.floor(self.selection_quad_.scale.y * self.h * 0.5)
            local x = w > 0 and self.selection_mouse_pos_.x or self.selection_mouse_pos_.x - math.abs(w)
            local y = h > 0 and (self.h - self.selection_mouse_pos_.y) or (self.h - self.selection_mouse_pos_.y) - math.abs(h)
            self.selection_fbo_:bind()
            
            if w ~= 0.0 or h ~= 0.0 then    
                local colors = self.selection_fbo_:read_colors(self.selection_color_tex_, gl.COLOR_ATTACHMENT0, x, y, math.abs(w), math.abs(h))
                self.editor_.tab_control_.selection_ = {}
                
                for i = 1, table.getn(colors) do
                    for j = 1, table.getn(self.sector_.objects_) do
                        if colors[i]:to_integer() == self.sector_.objects_[j].unique_color:to_integer() then
                            table.insert(self.editor_.tab_control_.selection_, self.sector_.objects_[j])
                            break
                        end
                    end
                end
            end
        end
        
        self.selection_visible_ = false
        self.is_dragging_ = false
        root.FrameBuffer.unbind()
    end
end

function editor_sector_view_base:draw_grid(size, subdivision)
    
    self.grid_shader_:bind() 
    gl.ActiveTexture(gl.TEXTURE0)
    root.Shader.get():sampler("s_Tex0", 0)
    self.grid_mesh_.scale = vec3(size)
    self.grid_tex_:bind()
    
    if self.sector_camera_.ortho and (self.sector_camera_.axis_ == math.PosX or self.sector_camera_.axis_ == math.NegX) then
        self.grid_mesh_.rotation = math.angle_axis(math.radians(90), vec3(0, 0, 1))
    end
    
    if self.sector_camera_.ortho and (self.sector_camera_.axis_ == math.PosZ or self.sector_camera_.axis_ == math.NegZ) then
        self.grid_mesh_.rotation = math.angle_axis(math.radians(90), vec3(1, 0, 0))
    end
    
    root.Shader.get():uniform("u_TexCoordScale", vec2(size))
    self.grid_mesh_:draw(self.sector_camera_)  
    root.Shader.get():uniform("u_TexCoordScale", vec2(size * subdivision))
    self.grid_mesh_:draw(self.sector_camera_) 
    
    root.Shader.get():uniform("u_TexCoordScale", vec2(1)) 
    root.Shader.pop_stack()
end

function editor_sector_view_base:draw_selection()

    if self.selection_visible_ then
    
        self.selection_shader_:bind()     
        root.Shader.get():uniform("u_ViewProjMatrix", mat4())
        root.Shader.get():uniform("u_EnableColor", 1)
        root.Shader.get():uniform("u_Color", root.Color.Red)
        root.Shader.get():uniform("u_Alpha", 0.5)
        self.selection_quad_:draw()
        
        root.Shader.get():uniform("u_EnableColor", 0)
        root.Shader.get():uniform("u_Alpha", 1.0)       
        root.Shader.pop_stack()
    end
end

function editor_sector_view_base:draw_manipulator(camera, size, color_pass)

    self.manipulator_shader_:bind()
    root.Shader.get():uniform("u_EnableColor", 1)
    gl.LineWidth(4.0)
    
    if not camera.ortho or (camera.axis_ ~= math.PosX and camera.axis_ ~= math.NegX) then
    
        -- x axis
        root.Shader.get():uniform("u_Color", color_pass and self.manipulator_axis_[1].unique_color or (self.editor_.tab_control_.current_axis_ == 1 and root.Color.Yellow or root.Color.Red))  
        self.cone_shape_.scale = vec3(size, size * 2.5, size)
        self.cone_shape_.rotation = math.angle_axis(math.radians(90.0), vec3(0, 0, 1))
        self.cone_shape_.position = self.manipulator_axis_[1].position
        if not color_pass then
            root.draw_line(camera, self.manipulator_center_, self.cone_shape_.position)
        end
        self.cone_shape_:draw(camera)
    end
    
    if not camera.ortho or (camera.axis_ ~= math.PosY and camera.axis_ ~= math.NegY) then
    
        -- y axis
        root.Shader.get():uniform("u_Color", color_pass and self.manipulator_axis_[2].unique_color or (self.editor_.tab_control_.current_axis_ == 2 and root.Color.Yellow or root.Color.Green))  
        self.cone_shape_.scale = vec3(size, size * 2.5, size)
        self.cone_shape_.rotation = quat()
        self.cone_shape_.position = self.manipulator_axis_[2].position
        if not color_pass then
            root.draw_line(camera, self.manipulator_center_, self.cone_shape_.position)
        end
        self.cone_shape_:draw(camera)
    end
    
    if not camera.ortho or (camera.axis_ ~= math.PosZ and camera.axis_ ~= math.NegZ) then
    
        -- z axis
        root.Shader.get():uniform("u_Color", color_pass and self.manipulator_axis_[3].unique_color or (self.editor_.tab_control_.current_axis_ == 3 and root.Color.Yellow or root.Color.Blue))   
        self.cone_shape_.scale = vec3(size, size * 2.5, size)
        self.cone_shape_.rotation = math.angle_axis(math.radians(90.0), vec3(1, 0, 0))
        self.cone_shape_.position = self.manipulator_axis_[3].position
        if not color_pass then
            root.draw_line(camera, self.manipulator_center_, self.cone_shape_.position)
        end
        self.cone_shape_:draw(camera)
    end
      
    root.Shader.get():uniform("u_EnableColor", 0)
    gl.LineWidth(1.0)  
    root.Shader.pop_stack()
end

function editor_sector_view_base:__render(skin)

    if self.editor_.scene_:is_planet_loaded() then
        self.planet_camera_:copy(self.sector_camera_)
        self.planet_camera_.position = self.sector_camera_.position + self.sector_.deformed_position_
        self.planet_camera_:refresh()

        self:render_scene(skin, function(planet_camera, sector_camera)
        
            gl.Enable(gl.DEPTH_TEST)
            gl.Enable(gl.CULL_FACE)
            
            if not sector_camera.ortho or (sector_camera.axis_ == math.PosY or sector_camera.axis_ == math.NegY) then
            
                self.terrain_shader_:bind()
                self.editor_.scene_:bind_global(planet_camera)
                
                self.sector_.terrain_:set_deformation(self.deformation_)
                self.sector_.terrain_:draw(sector_camera)
                self.sector_.terrain_:set_deformation(self.sector_.terrain_.spherical_deformation_)        
                root.Shader.pop_stack()
            end
            
            self.sector_:draw(sector_camera)

            gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
            gl.CullFace(gl.BACK)
            
            gl.Disable(gl.DEPTH_TEST)
            gl.Disable(gl.CULL_FACE)
        end)
        
        gl.BlendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
        gl.Enable(gl.BLEND)
        
        if self.sector_camera_.ortho then
            self:draw_grid(4096, 8)
        end
        
        if table.getn(self.editor_.tab_control_.selection_) ~= 0 then
        
            self.selection_shader_:bind()
            root.Shader.get():uniform("u_EnableColor", 1)
            root.Shader.get():uniform("u_Color", root.Color.Red)
        
            self.selection_bbox_:set(self.editor_.tab_control_.selection_[1].transform_.transfomed_box)       
            for i = 1, table.getn(self.editor_.tab_control_.selection_) do
            
                local transformed_box = self.editor_.tab_control_.selection_[i].transform_.transfomed_box
                root.draw_box(self.sector_camera_, transformed_box)
                self.selection_bbox_:enlarge(transformed_box)
            end
            
            gl.LineWidth(2.0)
            root.draw_box(self.sector_camera_, self.selection_bbox_)
            root.Shader.get():uniform("u_EnableColor", 0)
            root.Shader.pop_stack()
            gl.LineWidth(1.0)
            
            self.manipulator_axis_[1].position = self.selection_bbox_.center + vec3(-1, 0, 0)
            self.manipulator_axis_[2].position = self.selection_bbox_.center + vec3( 0, 1, 0)
            self.manipulator_axis_[3].position = self.selection_bbox_.center + vec3( 0, 0, 1)
            self.manipulator_center_ = self.selection_bbox_.center
            self:draw_manipulator(self.sector_camera_, 0.2, false)
        end
        
        self:draw_selection()
        gl.Disable(gl.BLEND)  
        self:end_render()
    end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor_sector_first_person_view implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'editor_sector_first_person_view' (editor_sector_view_base)

function editor_sector_first_person_view:__init(editor, parent, sector)
	editor_sector_view_base.__init(self, editor, parent, sector)

    self.sector_camera_ = camera_first_person(math.radians(90), 1.0, 0.1, 15000000.0)
    self.planet_camera_ = root.Camera()
    self.last_mouse_pos_ = vec2()
end

function editor_sector_first_person_view:process_event(event)

    editor_sector_view_base.process_event(self, event)
end

function editor_sector_first_person_view:process(dt)
    
    local mouse_x = sf.Mouse.get_position(self.editor_).x
    local mouse_y = sf.Mouse.get_position(self.editor_).y
    
    if self:is_hovered() then
        if sf.Mouse.is_button_pressed(sf.Mouse.Right) then
        
            local delta = vec2()
            delta.x = mouse_x - self.last_mouse_pos_.x
            delta.y = mouse_y - self.last_mouse_pos_.y
            self.sector_camera_:rotate(delta)
        end
        
        local speed = (sf.Keyboard.is_key_pressed(sf.Keyboard.LShift) and 15000.0 or 100.0) * dt
        if sf.Keyboard.is_key_pressed(sf.Keyboard.W) then
            self.sector_camera_:move_forward(speed)
        end
        if sf.Keyboard.is_key_pressed(sf.Keyboard.S) then
            self.sector_camera_:move_backward(speed)
        end
        if sf.Keyboard.is_key_pressed(sf.Keyboard.D) then
            self.sector_camera_:move_right(speed)
        end
        if sf.Keyboard.is_key_pressed(sf.Keyboard.A) then
            self.sector_camera_:move_left(speed)
        end
    end
    
    self.last_mouse_pos_ = vec2(mouse_x, mouse_y)
    self:redraw()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor_sector_ortho_view implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'editor_sector_ortho_view' (editor_sector_view_base)

function editor_sector_ortho_view:__init(editor, parent, sector, axis)
	editor_sector_view_base.__init(self, editor, parent, sector)

    self.sector_camera_ = camera_ortho(0, 0, 0, 0, -1000.0, 1000.0, axis)
    self.planet_camera_ = root.Camera()
    self.last_mouse_pos_ = vec2()
    
    self.render_pipeline_.enable_shadow_ = false
    self.render_pipeline_.enable_bloom_ = false
    self.render_pipeline_.enable_occlusion_ = false
    self.render_pipeline_.enable_lensflare_ = false
    self.render_pipeline_.enable_reflection_ = false
end

function editor_sector_ortho_view:process_event(event)

    if self:is_hovered() then
        if event.type == sf.Event.MouseWheelMoved then
           self.sector_camera_:zoom(event.mouse_wheel.delta)
        end
    end
    
    editor_sector_view_base.process_event(self, event)
end

function editor_sector_ortho_view:process(dt)
    
    local mouse_x = sf.Mouse.get_position(self.editor_).x
    local mouse_y = sf.Mouse.get_position(self.editor_).y
    
    if self:is_hovered() then
        if sf.Mouse.is_button_pressed(sf.Mouse.Right) then
        
            local delta = vec2()
            delta.x = mouse_x - self.last_mouse_pos_.x
            delta.y = mouse_y - self.last_mouse_pos_.y
            self.sector_camera_:drag(delta)
        end
    end
    
    self.sector_camera_.fixed_bounds_.x = -self.w * 0.5
    self.sector_camera_.fixed_bounds_.y =  self.w * 0.5
    self.sector_camera_.fixed_bounds_.z = -self.h * 0.5
    self.sector_camera_.fixed_bounds_.w =  self.h * 0.5    
    self.last_mouse_pos_ = vec2(mouse_x, mouse_y)
    self:redraw()
end