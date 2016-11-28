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
    
    self.button_strip_ = gwen.MenuStrip(self)
    self.button_strip_.h = 26
    
    local size = 24
    local border = 2
    
    self.button00_ = gwen.Button(self.button_strip_)
    self.button00_:set_bounds(border * 2, 1, size, size)
    
    self.button01_ = gwen.Button(self.button_strip_)
    self.button01_:set_bounds((self.button00_.x + self.button00_.w) + (border * 2), 1, size, size)
    self.button01_:set_is_toggle(true)
    
    self.button02_ = gwen.Button(self.button_strip_)
    self.button02_:set_bounds((self.button01_.x + self.button01_.w) + (border * 2), 1, size, size)
    self.button02_:set_disabled(true)
    
    self:init_menu_file()
    self:init_menu_client()
    
    self:dock(gwen.Base.Top)
    self.h = 50
end

function editor_menu:init_menu_file()

    self.menu_file_ = self.menu_strip_:add_item(gwen.MenuItem(self), "File")
    
    self.menu_file_new_ = self.menu_file_.menu:add_item(gwen.MenuItem(self), "New")
    
    self.menu_file_new_planet_ = self.menu_file_new_.menu:add_item(gwen.MenuItem(self), "Planet..", "", "Ctrl+Shift+N") 
    root.connect(self.menu_file_new_planet_.on_menu_item_selected, function(control)
        self.editor_:create_new_planet()
    end)
    
    self.menu_file_open_ = self.menu_file_.menu:add_item(gwen.MenuItem(self), "Open")
    root.connect(self.menu_file_open_.on_menu_item_selected, function(control)
        self.editor_:open_planet()
    end)
    
    self.menu_file_.menu:add_divider()
    
    self.menu_file_save_all_ = self.menu_file_.menu:add_item(gwen.MenuItem(self), "Save All")
    root.connect(self.menu_file_save_all_.on_menu_item_selected, function(control)
        self.editor_:save_all()
    end)
    
    self.menu_file_.menu:add_divider()
    
   	self.menu_file_exit_ = self.menu_file_.menu:add_item(gwen.MenuItem(self), "Exit", "", "Alt+F4")
	root.connect(self.menu_file_exit_.on_menu_item_selected, function(control)
		Engine:shutdown()
	end)
end

function editor_menu:init_menu_client()

    self.menu_client_ = self.menu_strip_:add_item(gwen.MenuItem(self), "Client")

    self.menu_client_run_ = self.menu_client_.menu:add_item(gwen.MenuItem(self), "Run")
    root.connect(self.menu_client_run_.on_menu_item_selected, function(control)
        self.editor_.client_entry_:set_allocate(true)
    end)
    
    self.menu_client_reload_ = self.menu_client_.menu:add_item(gwen.MenuItem(self), "Reload")
    root.connect(self.menu_client_reload_.on_menu_item_selected, function(control)
        self.editor_.client_entry_:reload()
    end)
    
    self.menu_client_close_ = self.menu_client_.menu:add_item(gwen.MenuItem(self), "Close")
    root.connect(self.menu_client_close_.on_menu_item_selected, function(control)
        self.editor_.client_entry_:set_allocate(false)
    end)
end