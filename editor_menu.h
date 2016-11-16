--[[
- @file editor_menu.h
- @brief
]]

class 'editor_menu' (gwen.MenuStrip)

function editor_menu:__init(editor, parent)
	gwen.MenuStrip.__init(self, parent)
	self.editor_ = editor
    
    self:init_menu_file()
    self:init_menu_client()
end

function editor_menu:init_menu_file()

    self.menu_file_ = self:add_item(gwen.MenuItem(self), "File")
    
    self.menu_file_new_ = self.menu_file_.menu:add_item(gwen.MenuItem(self), "New")
    
    self.menu_file_new_planet_ = self.menu_file_new_.menu:add_item(gwen.MenuItem(self), "Planet..", "", "Ctrl+Shift+N") 
    root.connect(self.menu_file_new_planet_.on_menu_item_selected, function(control)
        self.editor_:create_new_planet()
    end)
    
    self.menu_file_.menu:add_divider()
    
   	self.menu_file_exit_ = self.menu_file_.menu:add_item(gwen.MenuItem(self), "Exit", "", "Alt+F4")
	root.connect(self.menu_file_exit_.on_menu_item_selected, function(control)
		Engine:shutdown()
	end)
end

function editor_menu:init_menu_client()

    self.menu_client_ = self:add_item(gwen.MenuItem(self), "Client")

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