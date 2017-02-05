--[[
- @file editor_menu.h
- @brief
]]

class 'editor_menu' (gwen.Base)

function editor_menu:__init(editor, parent)
	gwen.Base.__init(self, parent)
	self.editor_ = editor
	
    self.menu_strip_ = gwen.MenuStrip(self)
	self.menu_strip_.h = 24
    self:init_menu_file()
	self:init_menu_client()
	self:init_menu_server()
	
	self.dock_strip_ = gwen.MenuStrip(self)
	self.dock_strip_.h = 26
	self.dock_dividers_ = {}
	self:init_dock_strip()
    
    self:dock(gwen.Base.Top)
    self.h = 50
end

function editor_menu:init_menu_file()

    self.menu_file_ = self.menu_strip_:add_item(gwen.MenuItem(self), "File")
	
	self.menu_file_new_ = self.editor_:add_item_atlas(self.menu_file_.menu, "New", "", 0, 0)
	
	self.menu_file_new_planet_ = self.editor_:add_item_atlas(self.menu_file_new_.menu, "Planet...", "Ctrl+Shift+N", 1, 0)
	root.connect(self.menu_file_new_planet_.on_menu_item_selected, function(control)
		self.editor_:create_new_planet()
	end)
	
	self.menu_file_new_sector_ = self.editor_:add_item_atlas(self.menu_file_new_.menu, "Sector...", "Alt+Shift+N", 8, 0)
	root.connect(self.menu_file_new_sector_.on_menu_item_selected, function(control)
		self.editor_:create_new_sector()
	end)
	
	self.menu_file_.menu:add_divider()
	
	self.menu_file_open_planet_ = self.editor_:add_item_atlas(self.menu_file_.menu, "Open Planet", "Ctrl+Shift+O", 7, 0)
	root.connect(self.menu_file_open_planet_.on_menu_item_selected, function(control)
		self.editor_:open_planet()
	end)
	
	self.menu_file_close_ = self.menu_file_.menu:add_item(gwen.MenuItem(self), "Close")
	root.connect(self.menu_file_close_.on_menu_item_selected, function(control)
		self.editor_:close_active()
	end)
	
	self.menu_file_close_planet_ = self.editor_:add_item_atlas(self.menu_file_.menu, "Close Planet", "", 6, 0)
	root.connect(self.menu_file_close_planet_.on_menu_item_selected, function(control)
		self.editor_:close_planet()
	end)
	
	self.menu_file_save_ = self.editor_:add_item_atlas(self.menu_file_.menu, "Save", "Ctrl+S", 10, 0)
	root.connect(self.menu_file_save_.on_menu_item_selected, function(control)
		self.editor_:save_active()
	end)
	
	self.menu_file_save_all_ = self.menu_file_.menu:add_item(gwen.MenuItem(self), "Save All", "", "Ctrl+Shift+S")
	root.connect(self.menu_file_save_all_.on_menu_item_selected, function(control)
		self.editor_:save_all()
	end)
	
    self.menu_file_.menu:add_divider()
    
   	self.menu_file_exit_ = self.editor_:add_item_atlas(self.menu_file_.menu, "Exit", "Alt+F4", 3, 0)
	root.connect(self.menu_file_exit_.on_menu_item_selected, function(control)
		self.editor_.status_bar_.text = "Shutting Down..."
		Engine:shutdown()
	end)
end

function editor_menu:init_menu_client()

    self.menu_client_ = self.menu_strip_:add_item(gwen.MenuItem(self), "Client")

    self.menu_client_run_ = self.menu_client_.menu:add_item(gwen.MenuItem(self), "Run")
    root.connect(self.menu_client_run_.on_menu_item_selected, function(control)
        self.editor_.client_entry_:set_allocate(true)
    end)
	
    self.menu_client_close_ = self.menu_client_.menu:add_item(gwen.MenuItem(self), "Close")
    root.connect(self.menu_client_close_.on_menu_item_selected, function(control)
        self.editor_.client_entry_:set_allocate(false)
    end)
end

function editor_menu:init_menu_server()

    self.menu_server_ = self.menu_strip_:add_item(gwen.MenuItem(self), "Server")

    self.menu_server_run_ = self.menu_server_.menu:add_item(gwen.MenuItem(self), "Run")
    root.connect(self.menu_server_run_.on_menu_item_selected, function(control)
        self.editor_.server_entry_:set_allocate(true)
    end)
	
    self.menu_server_close_ = self.menu_server_.menu:add_item(gwen.MenuItem(self), "Close")
    root.connect(self.menu_server_close_.on_menu_item_selected, function(control)
        self.editor_.server_entry_:set_allocate(false)
    end)
end

function editor_menu:init_dock_strip()

    self.dock_new_planet_ = gwen.Button(self.dock_strip_)
	self.dock_new_planet_:set_tooltip("New Planet (Ctrl+Shift+N)")
    self.dock_new_planet_:set_bounds(6, 1, 24, 24)
	self.editor_:set_button_atlas(self.dock_new_planet_, 2, 0)
	root.connect(self.dock_new_planet_.on_press, function(control)
		self.menu_file_new_planet_:press()
	end)
	
    self.dock_new_sector_ = gwen.Button(self.dock_strip_)
	self.dock_new_sector_:set_tooltip("New Sector (Alt+Shift+N)")
    self.dock_new_sector_:set_bounds(30, 1, 24, 24)
	self.editor_:set_button_atlas(self.dock_new_sector_, 9, 0)
	root.connect(self.dock_new_sector_.on_press, function(control)
		self.menu_file_new_sector_:press()
	end)
	
    self.dock_open_planet_ = gwen.Button(self.dock_strip_)
	self.dock_open_planet_:set_tooltip("Open Planet (Ctrl+Shift+O)")
    self.dock_open_planet_:set_bounds(54, 1, 24, 24)
	self.editor_:set_button_atlas(self.dock_open_planet_, 7, 0)
	root.connect(self.dock_open_planet_.on_press, function(control)
		self.menu_file_open_planet_:press()
	end)
	
	local divider = gwen.ImagePanel(self.dock_strip_)
	divider:set_bounds(82, 2, 1, 22)
	divider.color = root.Color.Black
	table.insert(self.dock_dividers_, divider)
	
	self.dock_selection_toggle_ = gwen.Button(self.dock_strip_)
	self.dock_selection_toggle_:set_tooltip("Selection")
	self.dock_selection_toggle_:set_bounds(86, 1, 24, 24)
	self.dock_selection_toggle_:set_is_toggle(true)
	self.dock_selection_toggle_:set_toggle_state(true)
	self.editor_:set_button_atlas(self.dock_selection_toggle_, 11, 0)
	root.connect(self.dock_selection_toggle_.on_toggle_off, function(control)
		self.dock_selection_toggle_:set_toggle_state(true)
	end)
	root.connect(self.dock_selection_toggle_.on_toggle_on, function(control)
		self.dock_terrain_edit_toggle_:set_toggle_state(false, false)
		self.dock_road_edit_toggle_:set_toggle_state(false, false)
	end)
	
	self.dock_terrain_edit_toggle_ = gwen.Button(self.dock_strip_)
	self.dock_terrain_edit_toggle_:set_tooltip("Terrain Edit")
	self.dock_terrain_edit_toggle_:set_bounds(112, 1, 24, 24)
	self.dock_terrain_edit_toggle_:set_is_toggle(true)
	self.editor_:set_button_atlas(self.dock_terrain_edit_toggle_, 12, 0)
	root.connect(self.dock_terrain_edit_toggle_.on_toggle_off, function(control)
		self.dock_terrain_edit_toggle_:set_toggle_state(true)
	end)
	root.connect(self.dock_terrain_edit_toggle_.on_toggle_on, function(control)
		self.dock_selection_toggle_:set_toggle_state(false, false)
		self.dock_road_edit_toggle_:set_toggle_state(false, false)
	end)
	
	self.dock_road_edit_toggle_ = gwen.Button(self.dock_strip_)
	self.dock_road_edit_toggle_:set_tooltip("Road Edit")
	self.dock_road_edit_toggle_:set_bounds(138, 1, 24, 24)
	self.dock_road_edit_toggle_:set_is_toggle(true)
	self.editor_:set_button_atlas(self.dock_road_edit_toggle_, 13, 0)
	root.connect(self.dock_road_edit_toggle_.on_toggle_off, function(control)
		self.dock_road_edit_toggle_:set_toggle_state(true)
	end)
	root.connect(self.dock_road_edit_toggle_.on_toggle_on, function(control)
		self.dock_selection_toggle_:set_toggle_state(false, false)
		self.dock_terrain_edit_toggle_:set_toggle_state(false, false)
	end)
end

function editor_menu:process_event(event)

	if event.type == sf.Event.KeyPressed then
		if event.key.control and event.key.shift and event.key.code == sf.Keyboard.N then
			self.menu_file_new_planet_:press()
		end
		if event.key.alt and event.key.shift and event.key.code == sf.Keyboard.N then
			self.menu_file_new_sector_:press()
		end
		if event.key.control and event.key.shift and event.key.code == sf.Keyboard.O then
			self.menu_file_open_planet_:press()
		end	
		if event.key.control and event.key.code == sf.Keyboard.S then
			self.menu_file_save_:press()
		end
		if event.key.control and event.key.shift and event.key.code == sf.Keyboard.S then
			self.menu_file_save_all_:press()
		end
	end
end

function editor_menu:refresh()

	self.menu_file_new_sector_.disabled = true
	self.menu_file_close_.disabled = true
	self.menu_file_close_planet_.disabled = true
	self.menu_file_save_.disabled = true
	self.menu_file_save_all_.disabled = true
	
	self.dock_selection_toggle_.disabled = true
	self.dock_terrain_edit_toggle_.disabled = true
	self.dock_road_edit_toggle_.disabled = true
	
	if self.editor_:is_planet_loaded() then
		self.menu_file_new_sector_.disabled = false
		self.menu_file_close_planet_.disabled = false
		self.menu_file_save_all_.disabled = false
		self.menu_file_save_.disabled = false
		
		local active_tab = self.editor_.tab_control_:get_active_tab()
		if active_tab then
			self.menu_file_close_.disabled = active_tab.is_planet_

			if not active_tab.is_planet_ then
				self.dock_selection_toggle_.disabled = false
				self.dock_terrain_edit_toggle_.disabled = false
				self.dock_road_edit_toggle_.disabled = false
			end
		end
	end
	
	self.dock_new_sector_.disabled = self.menu_file_new_sector_.disabled
end