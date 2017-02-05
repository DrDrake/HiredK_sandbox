--[[
- @file editor.lua
- @brief
]]

#include "common.h"

#include "editor_dialogs.h"
#include "editor_menu.h"
#include "editor_selection.h"
#include "editor_tab_control.h"
#include "editor_toolbox.h"
#include "editor_toolbox_objects.h"
#include "editor_toolbox_properties.h"
#include "editor_view.h"

class 'editor' (sf.Window)

function editor:__init(w, h, title, style)
    sf.Window.__init(self, sf.VideoMode(w, h), title, style, sf.ContextSettings(0, 0, 0))
    self.clear_color_ = root.Color(50, 50, 50, 255)
    self.event_ = sf.Event()
	
    self.canvas_ = gwen.Canvas(w, h, "images/skin_dark.png")
    self.canvas_.renderer.render_to_fbo = true
	
	self.client_entry_ = FileSystem:search("client.lua")
	self.server_entry_ = FileSystem:search("server.lua")
	self.planet_ = nil
	
	self.icon_atlas_ = FileSystem:search("images/icons.png", true)	
	self.menu_strip_ = editor_menu(self, self.canvas_)
	self.dock_base_ = gwen.DockBase(self.canvas_)
	self.dock_base_:dock(gwen.Base.Fill)
	
	self.toolbox_ = editor_toolbox(self, self.dock_base_)
	self.dock_base_:get_left():get_tab_control():add_page("Toolbox", self.toolbox_)
	self.dock_base_:set_left_size(350)
	
	self.selection_ = editor_selection(self)	
	
    self.tab_control_ = editor_tab_control(self, self.dock_base_)
    self.tab_control_:dock(gwen.Base.Fill)
	
    self.status_bar_ = gwen.StatusBar(self.canvas_)
	self.status_bar_.h = 24
	
	self:refresh_all()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor atlas utilities:
////////////////////////////////////////////////////////////////////////////////
]]

function editor:add_item_atlas(parent, name, accelerator, i, j)

	local u = 16 / self.icon_atlas_.w
	local v = 16 / self.icon_atlas_.h
	return parent:add_item(gwen.MenuItem(self.canvas_), name, self.icon_atlas_.path, accelerator, u * i, v * j, u * (i + 1), v * (j + 1), 16, 16)
end

function editor:add_node_atlas(parent, name, i, j)

	local u = 16 / self.icon_atlas_.w
	local v = 16 / self.icon_atlas_.h
	return parent:add_node(gwen.TreeNode(self.canvas_), name, self.icon_atlas_.path, u * i, v * j, u * (i + 1), v * (j + 1), 16, 16)
end

function editor:set_button_atlas(button, i, j)

	local u = 16 / self.icon_atlas_.w
	local v = 16 / self.icon_atlas_.h
	button:set_image(self.icon_atlas_.path, true, u * i, v * j, u * (i + 1), v * (j + 1), 16, 16)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor dialogs utilities:
////////////////////////////////////////////////////////////////////////////////
]]

function editor:create_confirm_dialog(title, text, cb)

	if self.confirm_dialog_ == nil or self.confirm_dialog_.hidden then
		self.confirm_dialog_ = confirm_dialog(self, self.canvas_, title, text, function()
		
			cb()
		
			self.confirm_dialog_:close_button_pressed()
			self.confirm_dialog_ = nil
			collectgarbage()
		end)
	end
end

function editor:create_save_file_dialog(title, exts, default_name, cb)

	if self.save_file_dialog_ == nil or self.save_file_dialog_.hidden then
		self.save_file_dialog_ = save_file_dialog(self, self.canvas_, title, exts, default_name, function(path)	
		
			cb(path)
            
			self.save_file_dialog_:close_button_pressed()
			self.save_file_dialog_ = nil
			collectgarbage()
		end)
	end
end

function editor:create_open_file_dialog(title, exts, cb)

	if self.open_file_dialog_ == nil or self.open_file_dialog_.hidden then
		self.open_file_dialog_ = open_file_dialog(self, self.canvas_, title, exts, function(path)	
		
			cb(path)
            
			self.open_file_dialog_:close_button_pressed()
			self.open_file_dialog_ = nil
			collectgarbage()
		end)
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor dropdown utilities:
////////////////////////////////////////////////////////////////////////////////
]]

function editor:create_dropdown_menu(options, cb)

	local mouse_pos = self.canvas_:canvas_pos_to_local(gwen.Point(
		sf.Mouse.get_position(self).x,
		sf.Mouse.get_position(self).y
	))

	self:close_dropdown_menu()
	self.dropdown_panel_ = gwen.ImagePanel(self.canvas_)
	self.dropdown_panel_.color = root.Color(100, 100, 100, 200)
	self.dropdown_panel_.x = mouse_pos.x
	self.dropdown_panel_.y = mouse_pos.y
	self.dropdown_panel_.w = 0
	self.dropdown_panel_.h = 0
	
	self.dropdown_panel_.focused_ = false
	self.dropdown_panel_.items_ = {}
	
	for i = 1, table.getn(options) do
	
		self.dropdown_panel_.items_[i] = gwen.MenuItem(self.dropdown_panel_)
		self.dropdown_panel_.items_[i]:set_alignment(bit.bor(gwen.Base.CenterV, gwen.Base.Left))
		self.dropdown_panel_.items_[i]:set_text(options[i])
		self.dropdown_panel_.items_[i].y = (i - 1) * self.dropdown_panel_.items_[i].h
		self.dropdown_panel_.w = math.max(self.dropdown_panel_.w, self.dropdown_panel_.items_[i].w)
		self.dropdown_panel_.h = self.dropdown_panel_.h + self.dropdown_panel_.items_[i].h
		
		root.connect(self.dropdown_panel_.items_[i].on_hover_enter, function(control)
			self.dropdown_panel_.focused_ = true
		end)
		root.connect(self.dropdown_panel_.items_[i].on_hover_leave, function(control)
			self.dropdown_panel_.focused_ = false
		end)
		root.connect(self.dropdown_panel_.items_[i].on_press, function(control)
			cb(self.dropdown_panel_.items_[i].text)
			self.dropdown_panel_.focused_ = false
			self:close_dropdown_menu()
		end)
	end
end

function editor:close_dropdown_menu()
	if self.dropdown_panel_ ~= nil and self.dropdown_panel_.focused_ == false then
		self.dropdown_panel_:hide()
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor textbox_edit utilities:
////////////////////////////////////////////////////////////////////////////////
]]

function editor:create_textbox_edit(node, cb)

	self:close_textbox_edit()
	self.textbox_edit_ = gwen.TextBox(self.canvas_)	
	self.textbox_edit_.x = node:local_pos_to_canvas().x + 31
	self.textbox_edit_.y = node:local_pos_to_canvas().y + 1
	self.textbox_edit_:set_size(node.w - 31, 16)	
	
	self.textbox_edit_:set_text(node.text)
	self.textbox_edit_:select_all()
	self.textbox_edit_.focused_ = false
	
	root.connect(self.textbox_edit_.on_hover_enter, function(control)
		self.textbox_edit_.focused_  = true
	end)
	root.connect(self.textbox_edit_.on_hover_leave, function(control)
		self.textbox_edit_.focused_  = false
	end)	
	root.connect(self.textbox_edit_.on_return_press, function(control)
		cb(self.textbox_edit_.text)
		self.textbox_edit_:hide()
		self.textbox_edit_ = nil
		collectgarbage()
	end)
end

function editor:close_textbox_edit()

	if self.textbox_edit_ ~= nil and self.textbox_edit_.focused_ == false then
		self.textbox_edit_:hide()
		self.textbox_edit_ = nil
		collectgarbage()
	end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// editor implementation:
////////////////////////////////////////////////////////////////////////////////
]]

function editor:is_planet_loaded()
	return (self.planet_ ~= nil) and true or false
end

function editor:create_new_planet()

	if self.new_planet_dialog_ == nil or self.new_planet_dialog_.hidden then
		self.new_planet_dialog_ = new_planet_dialog(self, self.canvas_, function(name, radius, seed)
        
            local default_name = string.gsub(string.lower(name), " ", "_") .. ".json"
            self:create_save_file_dialog("Save Planet As..", { "json" }, default_name, function(path)
			
				self:close_planet()				
				self.planet_ = planet(path)
				self.planet_:build(radius, 14, seed)
				self.planet_.name = name				
				self.planet_:write()
				
				self.planet_.ocean_.disabled_ = true

				self.tab_control_:add_planet_tab()
				self:refresh_all()
            end)

			self.new_planet_dialog_:close_button_pressed()
			self.new_planet_dialog_ = nil
			collectgarbage()
		end)
	end
end

function editor:create_new_sector()

	if self.new_sector_dialog_ == nil or self.new_sector_dialog_.hidden then
		self.new_sector_dialog_ = new_sector_dialog(self, self.canvas_, function(name, coord, face)
		
			local default_name = string.gsub(string.lower(name), " ", "_") .. ".json"
			self:create_save_file_dialog("Save Sector As..", { "json" }, default_name, function(path)
			
				local new_sector = planet_sector(path, self.planet_)
				new_sector.name = name
				new_sector.coord_ = coord
				new_sector.face_ = face
				new_sector:write()
				
				table.insert(self.planet_.loaded_sectors_, new_sector)
				self.tab_control_:add_sector_tab(new_sector)
				self.planet_:rebuild_sectors_cloud()
				self.planet_:write()				
				self:refresh_all()
			end)
		
			self.new_sector_dialog_:close_button_pressed()
			self.new_sector_dialog_ = nil
			collectgarbage()
		end)
	end
end

function editor:delete_sector(sector)

	for i = 1, table.getn(self.planet_.loaded_sectors_) do
		if self.planet_.loaded_sectors_[i] == sector then
		
			local text = string.format("Are you sure you want to delete the selected file? ('%s')", sector.path_)
			self:create_confirm_dialog("Confirm Delete", text, function()
			
				self.tab_control_:remove_sector_tab(sector)		
				table.remove(self.planet_.loaded_sectors_, i)
				root.FileSystem.delete(sector.path_)
				collectgarbage()
				
				self.planet_:rebuild_sectors_cloud()
				self.planet_:write()
				self:refresh_all()
			end)
			
			break
		end			
	end
end

function editor:open_planet()

	self:create_open_file_dialog("Open Planet", { "json" }, function(path)	
	
		self:close_planet()
		self.planet_ = planet(path)
		self.planet_:parse()
		
		self.planet_.ocean_.disabled_ = true

		self.tab_control_:add_planet_tab(self.planet_)
		self:refresh_all()
	end)
end

function editor:close_active()

	if self:is_planet_loaded() then
		
		local tab = self.tab_control_:get_active_tab()
		self.tab_control_:remove_tab(tab)
		self:refresh_all()
	end
end

function editor:close_planet()

	if self:is_planet_loaded() then
	
		self.tab_control_:clear()
		self.planet_ = nil
		collectgarbage()
		self:refresh_all()
	end
end

function editor:save_active()

	if self:is_planet_loaded() then
	
		local tab = self.tab_control_:get_active_tab()
		if tab.is_planet_ then
			self.planet_:write()
		else
			tab.sector_:write()
		end
		
		self:refresh_all()
	end
end

function editor:save_all()

	if self:is_planet_loaded() then
	
		self.planet_:write()
		for i = 2, table.getn(self.tab_control_.tabs_) do
			local sector = self.tab_control_.tabs_[i].sector_
			sector:write()
		end
		
		self:refresh_all()
	end
end

function editor:refresh_all()

	if self:is_planet_loaded() then
        self:set_title(self.planet_.name .. " - " .. EDITOR_WINDOW_TITLE)
	else
		self:set_title(EDITOR_WINDOW_TITLE)
	end
	
	self.menu_strip_:refresh()
	self.toolbox_:refresh()
	self.selection_:refresh()
	self.tab_control_:refresh()
end

function editor:__update(dt)

    while self:poll_event(self.event_) do
        if self.event_.type == sf.Event.Closed then
            Engine:shutdown()
            return
        end
		
        if self.event_.type == sf.Event.Resized then
            self.canvas_.w = self.event_.size.w
            self.canvas_.h = self.event_.size.h
			self:refresh_all()
        end
		
		if self.event_.type == sf.Event.MouseButtonPressed then		   
			self:close_dropdown_menu()
			self:close_textbox_edit()
		end
        
		self.menu_strip_:process_event(self.event_)
		self.toolbox_:process_event(self.event_)
		self.tab_control_:process_event(self.event_)
        self.canvas_:process_event(self.event_)
    end
    
    self:set_active(true)  
    gl.ClearColor(self.clear_color_)
    gl.Clear(gl.COLOR_BUFFER_BIT)
	self.canvas_:draw()
    self:display()
end

instance = editor(1200, 800, EDITOR_WINDOW_TITLE, sf.Window.Default)