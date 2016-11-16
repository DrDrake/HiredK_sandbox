--[[
- @file editor_toolbox.h
- @brief
]]

class 'editor_toolbox' (gwen.Base)

function editor_toolbox:__init(editor, parent)
	gwen.Base.__init(self, parent)
	self.editor_ = editor
    
    self.bottom_control_ = gwen.ResizableControl(self)
	self.bottom_control_:dock(gwen.Base.Bottom)
 	self.bottom_control_.margin_right = -6
	self.bottom_control_.margin_left = -6
	self.bottom_control_.margin_bottom = -6
	self.bottom_control_.h = 300
    
	self.sector_tree_control_ = gwen.TreeControl(self.bottom_control_)
	self.sector_tree_control_:dock(gwen.Base.Fill)
end

function editor_toolbox:refresh_gui(w, h)

    self.sector_tree_control_:clear()
    
    if self.editor_.scene_:is_planet_loaded() then
    
        local planet_node = self.sector_tree_control_:add_node(gwen.TreeNode(self), "Planet")
        root.connect(planet_node.on_double_click, function(control)
            self.editor_.tab_control_.planet_tab_:press()
        end)
        
        for i = 1, table.getn(self.editor_.scene_.planet_.loaded_sectors_) do
        
            local sector_node = planet_node:add_node(gwen.TreeNode(self), "Sector")
            
            root.connect(sector_node.on_double_click, function(control)
                local sector = self.editor_.scene_.planet_.loaded_sectors_[i]
                self.editor_.tab_control_:add_sector_tab(sector)
            end)
        end
        
        planet_node:expand_all()
    end
end