--[[
- @file editor_tab_control.h
- @brief
]]

class 'editor_tab_control' (gwen.TabControl)

function editor_tab_control:__init(editor, parent)
	gwen.TabControl.__init(self, parent)
    self.type = root.ScriptObject.Dynamic
	self.editor_ = editor
    self.hidden = true   
    
    self.planet_tab_ = nil
    self.sector_tabs_ = {}
end

function editor_tab_control:add_sector_tab(sector)

    for i = 1, table.getn(self.sector_tabs_) do
        if self.sector_tabs_[i].sector_.index_ == sector.index_ then
            self.sector_tabs_[i]:press()
            return self.sector_tabs_[i]
        end
    end
    
    local sector_tab = self:add_page(gwen.TabButton(self), "Sector")
    sector_tab.view_ = editor_sector_view(self.editor_, sector_tab.page, sector)
    sector_tab.view_:refresh_gui(self.editor_.w, self.editor_.h)
    sector_tab.view_:dock(gwen.Base.Fill)
    
    sector_tab.sector_ = sector
    sector_tab.sector_:page_in()
    
    table.insert(self.sector_tabs_, sector_tab)
    sector_tab:press()
    return sector_tab
end

function editor_tab_control:process_event(event)

    if self.editor_:has_focus() and self.editor_.scene_:is_planet_loaded() then
    
        if self.planet_tab_:is_active() then
            self.planet_tab_.view_:process_event(event)
        end
    end
end

function editor_tab_control:__update(dt)

    if self.editor_:has_focus() and self.editor_.scene_:is_planet_loaded() then
    
        if self.planet_tab_:is_active() then
            self.planet_tab_.view_:process(dt)
            return
        end
        
        for i = 1, table.getn(self.sector_tabs_) do
            if self.sector_tabs_[i]:is_active() then
                self.sector_tabs_[i].view_:process(dt)
                return
            end
        end
    end
end

function editor_tab_control:refresh_gui(w, h)

    if self.editor_.scene_:is_planet_loaded() then

        if self.planet_tab_ == nil then       
            self.planet_tab_ = self:add_page(gwen.TabButton(self), "Planet")
            self.planet_tab_.view_ = editor_planet_view(self.editor_, self.planet_tab_.page)
            self.planet_tab_.view_:dock(gwen.Base.Fill)
        end
        
        for i = 1, table.getn(self.sector_tabs_) do
            self.sector_tabs_[i].view_:refresh_gui(w, h)
        end
        
        self.planet_tab_.view_:refresh_gui(w, h)
        self.hidden = false
    end
end