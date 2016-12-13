--[[
- @file editor_menu.h
- @brief
]]

class 'editor_menu' (gwen.Base)

function editor_menu:__init(editor, parent)
	gwen.Base.__init(self, parent)
	self.editor_ = editor
    
    self.menu_strip_ = gwen.MenuStrip(self)
    self:init_menu_file()
    
    self:dock(gwen.Base.Fill)
end

function editor_menu:init_menu_file()

    self.menu_file_ = self.menu_strip_:add_item(gwen.MenuItem(self), "File")
    
    self.menu_file_new_planet_ = self.menu_file_.menu:add_item(gwen.MenuItem(self), "New") 
    root.connect(self.menu_file_new_planet_.on_menu_item_selected, function(control)
        self.editor_:create_new_planet()
    end)
    
    self.menu_file_.menu:add_divider()
    
   	self.menu_file_exit_ = self.menu_file_.menu:add_item(gwen.MenuItem(self), "Exit", "", "Alt+F4")
	root.connect(self.menu_file_exit_.on_menu_item_selected, function(control)
		Engine:shutdown()
	end)
end