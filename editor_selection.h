--[[
- @file editor_selection.h
- @brief
]]

class 'editor_selection' (root.ScriptObject)

function editor_selection:__init(editor)
	root.ScriptObject.__init(self, "selection")
	self.editor_ = editor
	
	self.selection_mesh_ = root.Mesh.build_cube_wire()
	self.selection_bbox_ = Box3D()
	self.selected_objects_ = {}
	
    self.manipulator_axis_ = {}
    self.manipulator_axis_[1] = root.Transform() -- x
    self.manipulator_axis_[2] = root.Transform() -- y
    self.manipulator_axis_[3] = root.Transform() -- z
    self.manipulator_center_ = vec3()
	self.active_axis_ = 0
	
	self.drag_plane_ = root.Mesh.build_quad()
	self.drag_position_ = vec3()
	
	self.cone_mesh_ = root.Mesh.build_cone(1.0, 1.0)
	self.selection_quad_ = root.Mesh.build_quad()
	self.selection_quad_.scale = vec3()
	self.pick_mouse_pos_ = gwen.Point()
	self.is_dragging_ = false
	self.match_found_ = false
end

function editor_selection:refresh()

	if self.picking_depth_tex_ ~= nil and self.picking_depth_tex_.w == self.editor_.w and self.picking_depth_tex_.h == self.editor_.h then
		return
	end

    self.picking_fbo_ = root.FrameBuffer()
    self.picking_depth_tex_ = root.Texture.from_empty(self.editor_.w, self.editor_.h, { iformat = gl.DEPTH_COMPONENT32F, format = gl.DEPTH_COMPONENT, type = gl.FLOAT })
    self.picking_color_tex_ = root.Texture.from_empty(self.editor_.w, self.editor_.h)
end

function editor_selection:unproject(camera, x, y, w, h)

	local viewport = vec4()
	viewport.z = w
	viewport.w = h

	local depth = self.picking_fbo_:read_pixel(self.picking_depth_tex_, gl.DEPTH_COMPONENT, x, y).x
    return math.unproject(vec3(x, y, depth), camera.view_matrix, camera.proj_matrix, viewport)
end

function editor_selection:compute_drag_position(camera, x, y, w, h)

    self.picking_fbo_:clear()
    self.picking_fbo_:attach(self.picking_depth_tex_, gl.DEPTH_ATTACHMENT)
    self.picking_fbo_:bind_output()
    
    gl.Clear(gl.DEPTH_BUFFER_BIT)
	gl.PushAttrib(gl.VIEWPORT_BIT)
	gl.Viewport(0, 0, w, h)
	
	gl.Enable(gl.DEPTH_TEST)
	
	if self.active_axis_ == 1 then
		self.drag_plane_.rotation = quat()
	end
	
	if self.active_axis_ == 2 then
		self.drag_plane_.rotation = math.angle_axis(math.radians(90), vec3(0, 0, 1))
	end
    
    if self.active_axis_ == 3 then
        self.drag_plane_.rotation = math.angle_axis(math.radians(90), vec3(1, 0, 0))
    end
	
	self.drag_plane_.scale = vec3(4096.0)
    self.drag_plane_:draw(camera)
	
	gl.Disable(gl.DEPTH_TEST)
	gl.PopAttrib()
	
	local pos = self:unproject(camera, x, y, w, h)
	root.FrameBuffer.unbind()
	return pos
end

function editor_selection:pick_selection(camera, x, y, w, h)

	self.selection_quad_.position.x = ((x * (1.0 / w)) * 2.0 - 1.0) + self.selection_quad_.scale.x
	self.selection_quad_.position.y = ((1.0 - (y * (1.0 / h))) * 2.0 - 1.0) - self.selection_quad_.scale.y
	self.selection_quad_.scale = vec3()
	self.match_found_ = false
	
	self.picking_fbo_:clear()
	self.picking_fbo_:attach(self.picking_depth_tex_, gl.DEPTH_ATTACHMENT)
	self.picking_fbo_:attach(self.picking_color_tex_, gl.COLOR_ATTACHMENT0)
	self.picking_fbo_:bind_output()
	
	gl.ClearColor(root.Color.Black)
	gl.Clear(bit.bor(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT))
	gl.PushAttrib(gl.VIEWPORT_BIT)
	gl.Viewport(0, 0, w, h)
	
	gl.Enable(gl.DEPTH_TEST)
	
	for i = 1, table.getn(self.editor_.planet_.active_sectors_) do
	
		-- write active objects unique color to fbo
		local sector = self.editor_.planet_.active_sectors_[i]
		for j = 1, table.getn(sector.objects_) do
		
			if sector.objects_[j].mesh_ ~= nil then
				root.Shader.get():uniform("u_RenderingType", 4)
				root.Shader.get():uniform("u_Color", sector.objects_[j].unique_color)
				sector.objects_[j].mesh_:draw(camera)
			end
		end
	end
	
	gl.Clear(gl.DEPTH_BUFFER_BIT)
	local scale_factor = math.clamp(math.distance(camera.position, self.selection_mesh_.position), 20, 500) * 0.025
	self:draw_manipulator(camera, scale_factor, true)
	gl.Disable(gl.DEPTH_TEST)
	
	local pixel = self.picking_fbo_:read_color(self.picking_color_tex_, gl.COLOR_ATTACHMENT0, x, h - y)
	
	for i = 1, table.getn(self.manipulator_axis_) do
		if pixel:to_integer() == self.manipulator_axis_[i].unique_color:to_integer() then
		
			self.drag_plane_.position = self.manipulator_axis_[i].position
			self.drag_position_ = self:compute_drag_position(camera, x, h - y, w, h)
			self.active_axis_ = i
			self.match_found_ = true
			break
		end
	end
	
	for i = 1, table.getn(self.editor_.planet_.active_sectors_) do
	
		local sector = self.editor_.planet_.active_sectors_[i]
		for j = 1, table.getn(sector.objects_) do
			if sector.objects_[j].unique_color:to_integer() == pixel:to_integer() then
			
				local is_selected = self:is_selected(sector.objects_[j])
				local multiselect = self:is_multiselect()
				local cleared = false
				
				if not sf.Keyboard.is_key_pressed(sf.Keyboard.LControl) then
					self:clear()
					cleared = true
				end
			
				if not is_selected or (multiselect and cleared) then
					self:add_to_selected(sector.objects_[j])
				else
					self:remove_selected(sector.objects_[j])
				end
				
				self.editor_:refresh_all()
				self.match_found_ = true
				break
			end
		end
	end
	
	self.pick_mouse_pos_ = gwen.Point(x, y)
	self.is_dragging_ = false
	root.FrameBuffer.unbind()
	gl.PopAttrib()
end

function editor_selection:drag_selection(camera, x, y, w, h)

	if self.active_axis_ ~= 0 then
	
		local new_drag_position = self:compute_drag_position(camera, x, h - y, w, h)
		local delta = self.drag_position_ - new_drag_position
		
		for i = 1, table.getn(self.selected_objects_) do
			local transform = self.selected_objects_[i].transform_
			
			if self.active_axis_ == 1 then
				transform.position.x = transform.position.x - delta.x
			end
			if self.active_axis_ == 2 then
				transform.position.y = transform.position.y - delta.y
			end
			if self.active_axis_ == 3 then
				transform.position.z = transform.position.z - delta.z
			end
		end
		
		self.drag_position_ = new_drag_position
		self.editor_.toolbox_.properties_tab_.page_:on_selection_update()
		self:rebuild_selection_bbox()
		return
	end
	
	self.selection_quad_.scale.x = (self.pick_mouse_pos_.x - x) * (1.0 / w)
	self.selection_quad_.scale.y = (self.pick_mouse_pos_.y - y) * (1.0 / h)
	self.selection_quad_.position.x = ((x * (1.0 / w)) * 2.0 - 1.0) + self.selection_quad_.scale.x
	self.selection_quad_.position.y = ((1.0 - (y * (1.0 / h))) * 2.0 - 1.0) - self.selection_quad_.scale.y
	self.is_dragging_ = true
end

function editor_selection:release_selection(w, h)

	if self.is_dragging_ then
	
		local sw = math.floor(self.selection_quad_.scale.x * w)
		local sh = math.floor(self.selection_quad_.scale.y * h)	
		local sx = sw < 0 and self.pick_mouse_pos_.x or self.pick_mouse_pos_.x - math.abs(sw)
		local sy = sh > 0 and h - self.pick_mouse_pos_.y or (h - self.pick_mouse_pos_.y) - math.abs(sh)
	
		local colors = self.picking_fbo_:read_colors(self.picking_color_tex_, gl.COLOR_ATTACHMENT0, sx, sy, math.abs(sw), math.abs(sh))
		self:clear()
		
		for i = 1, table.getn(colors) do		
			for j = 1, table.getn(self.editor_.planet_.active_sectors_) do		
				local sector = self.editor_.planet_.active_sectors_[j]
				for k = 1, table.getn(sector.objects_) do
					if sector.objects_[k].unique_color:to_integer() == colors[i]:to_integer() then
						self:add_to_selected(sector.objects_[k])
						self.match_found_ = true
						break
					end
				end
			end
		end
		
		self.editor_:refresh_all()
		self.is_dragging_ = false
		root.FrameBuffer.unbind()
	end
	
	if not self.match_found_ then
		self:clear()
		self.editor_:refresh_all()
	end

	self.selection_quad_.scale = vec3()
	self.active_axis_ = 0
end

function editor_selection:rebuild_selection_bbox()

	if table.getn(self.selected_objects_) > 0 then
		self.selection_bbox_:set(self.selected_objects_[1].transform_.transfomed_box)
		for i = 1, table.getn(self.selected_objects_) do
		
			local tb = self.selected_objects_[i].transform_.transfomed_box
			self.selection_bbox_:enlarge(tb)
		end
	end
end

function editor_selection:is_multiselect()
	return table.getn(self.selected_objects_) > 1
end

function editor_selection:is_selected(object)
	for i = 1, table.getn(self.selected_objects_) do
		if self.selected_objects_[i] == object then
			return true
		end
	end
	
	return false
end

function editor_selection:add_to_selected(object)
	table.insert(self.selected_objects_, object)
	self.editor_.toolbox_.properties_tab_.page_:on_selection_update()
	self:rebuild_selection_bbox()
end

function editor_selection:remove_selected(object)

	for i = 1, table.getn(self.selected_objects_) do
		if self.selected_objects_[i] == object then		
			table.remove(self.selected_objects_, i)
			self.editor_.toolbox_.properties_tab_.page_:on_selection_update()
			self:rebuild_selection_bbox()
			break
		end
	end
end

function editor_selection:delete_selected_objects()

	for i = 1, table.getn(self.selected_objects_) do
		for j = 1, table.getn(self.editor_.planet_.active_sectors_) do
		
			local sector = self.editor_.planet_.active_sectors_[j]
			for k = table.getn(sector.objects_), 1, -1 do
			
				if self:is_selected(sector.objects_[k]) then		
					self:remove_selected(sector.objects_[k])
					table.remove(sector.objects_, k)
					collectgarbage()
				end
			end
		end
	end

	self.editor_:refresh_all()
end

function editor_selection:draw_manipulator(camera, size, color_pass)

	root.Shader.get():uniform("u_RenderingType", 4)
	gl.LineWidth(2.0)
	
	self.manipulator_axis_[1].position = self.selection_bbox_.center + vec3(-5, 0, 0) * size
	self.manipulator_axis_[2].position = self.selection_bbox_.center + vec3( 0, 5, 0) * size
	self.manipulator_axis_[3].position = self.selection_bbox_.center + vec3( 0, 0, 5) * size
	self.manipulator_center_ = self.selection_bbox_.center
	
	if not camera.ortho or (camera.axis_ ~= math.PosX and camera.axis_ ~= math.NegX) then
	
		self.cone_mesh_.position = self.manipulator_axis_[1].position
		self.cone_mesh_.rotation = math.angle_axis(math.radians(90.0), vec3(0, 0, 1))
		self.cone_mesh_.scale = vec3(size, size * 2.5, size)
		
		local color = color_pass and self.manipulator_axis_[1].unique_color or (self.active_axis_ == 1 and root.Color.Yellow or root.Color.Red)
		root.Shader.get():uniform("u_Color", color)
		
		root.Mesh.draw_line(camera, self.manipulator_center_, self.cone_mesh_.position)
		self.cone_mesh_:draw(camera)
	end
	
	if not camera.ortho or (camera.axis_ ~= math.PosY and camera.axis_ ~= math.NegY) then
	
		self.cone_mesh_.position = self.manipulator_axis_[2].position
		self.cone_mesh_.rotation = quat()
		self.cone_mesh_.scale = vec3(size, size * 2.5, size)
		
		local color = color_pass and self.manipulator_axis_[2].unique_color or (self.active_axis_ == 2 and root.Color.Yellow or root.Color.Green)
		root.Shader.get():uniform("u_Color", color)
		
		root.Mesh.draw_line(camera, self.manipulator_center_, self.cone_mesh_.position)
		self.cone_mesh_:draw(camera)
	end
	
	if not camera.ortho or (camera.axis_ ~= math.PosZ and camera.axis_ ~= math.NegZ) then
	
		self.cone_mesh_.position = self.manipulator_axis_[3].position
		self.cone_mesh_.rotation = math.angle_axis(math.radians(90.0), vec3(1, 0, 0))
		self.cone_mesh_.scale = vec3(size, size * 2.5, size)
		
		local color = color_pass and self.manipulator_axis_[3].unique_color or (self.active_axis_ == 3 and root.Color.Yellow or root.Color.Blue)
		root.Shader.get():uniform("u_Color", color)
		
		root.Mesh.draw_line(camera, self.manipulator_center_, self.cone_mesh_.position)
		self.cone_mesh_:draw(camera)
	end
	
	gl.LineWidth(1.0)
end

function editor_selection:draw(camera)

	local draw_selection_bbox = function()
	
		self.selection_mesh_.position = self.selection_bbox_.center
		self.selection_mesh_.scale.x = self.selection_bbox_.max.x - self.selection_bbox_.min.x
		self.selection_mesh_.scale.y = self.selection_bbox_.max.y - self.selection_bbox_.min.y
		self.selection_mesh_.scale.z = self.selection_bbox_.max.z - self.selection_bbox_.min.z		
		root.Shader.get():uniform("u_RenderingType", 1)
		self.selection_mesh_:draw(camera)
	end
	
	local draw_selection_quad = function()
	
        root.Shader.get():uniform("u_ViewProjMatrix", mat4())
        root.Shader.get():uniform("u_RenderingType", 4)
        root.Shader.get():uniform("u_Color", root.Color(255, 0, 0, 100))
		
		gl.Enable(gl.BLEND)
        self.selection_quad_:draw()
		gl.Disable(gl.BLEND)
	end

	if table.getn(self.selected_objects_) > 0 then
		draw_selection_bbox()
		
		local scale_factor = math.clamp(math.distance(camera.position, self.selection_mesh_.position), 20, 500) * 0.025
		self:draw_manipulator(camera, scale_factor, false)
	end
	
	draw_selection_quad()
end

function editor_selection:clear()

	self.selected_objects_ = {}
end