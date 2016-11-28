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
    
    self.current_axis_ = 0
    self.selection_ = {}
end

function editor_tab_control:add_sector_tab(sector)

    for i = 1, table.getn(self.sector_tabs_) do
        if self.sector_tabs_[i].sector_.index_ == sector.index_ then
            self.sector_tabs_[i]:press()
            return self.sector_tabs_[i]
        end
    end
    
    local sector_tab = self:add_page(gwen.TabButton(self), "Sector")
    sector_tab.cross_splitter_ = gwen.CrossSplitter(sector_tab.page)
    sector_tab.cross_splitter_:dock(gwen.Base.Fill)
    
    sector_tab.view0_ = editor_sector_first_person_view(self.editor_, self, sector)
    sector_tab.view0_:refresh_gui(self.editor_.w, self.editor_.h)
    sector_tab.cross_splitter_:set_panel(0, sector_tab.view0_)
    
    sector_tab.view1_ = editor_sector_ortho_view(self.editor_, self, sector, math.NegY)
    sector_tab.view1_:refresh_gui(self.editor_.w, self.editor_.h)
    sector_tab.cross_splitter_:set_panel(1, sector_tab.view1_)
    
    sector_tab.view2_ = editor_sector_ortho_view(self.editor_, self, sector, math.NegX)
    sector_tab.view2_:refresh_gui(self.editor_.w, self.editor_.h)
    sector_tab.cross_splitter_:set_panel(2, sector_tab.view2_)
    
    sector_tab.view3_ = editor_sector_ortho_view(self.editor_, self, sector, math.NegZ)
    sector_tab.view3_:refresh_gui(self.editor_.w, self.editor_.h)
    sector_tab.cross_splitter_:set_panel(3, sector_tab.view3_)
    
    sector_tab.sector_ = sector
    
    table.insert(self.editor_.scene_.planet_.active_sectors_, sector)
    sector_tab.sector_:page_in()
    self.editor_:refresh_gui()
    
    table.insert(self.sector_tabs_, sector_tab)
    sector_tab:press()
    return sector_tab
end

function editor_tab_control:process_event(event)

    if self.editor_:has_focus() and self.editor_.scene_:is_planet_loaded() then
    
        if self.planet_tab_:is_active() then
            self.planet_tab_.view_:process_event(event)
            return
        end
        
        for i = 1, table.getn(self.sector_tabs_) do
            if self.sector_tabs_[i]:is_active() then
                self.sector_tabs_[i].view0_:process_event(event)
                self.sector_tabs_[i].view1_:process_event(event)
                self.sector_tabs_[i].view2_:process_event(event)
                self.sector_tabs_[i].view3_:process_event(event)
                return
            end
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
                self.sector_tabs_[i].view0_:process(dt)
                self.sector_tabs_[i].view1_:process(dt)
                self.sector_tabs_[i].view2_:process(dt)
                self.sector_tabs_[i].view3_:process(dt)
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
            self.planet_tab_:press()
        end
        
        for i = 1, table.getn(self.sector_tabs_) do
            self.sector_tabs_[i].view0_:refresh_gui(w, h)
            self.sector_tabs_[i].view1_:refresh_gui(w, h)
            self.sector_tabs_[i].view2_:refresh_gui(w, h)
            self.sector_tabs_[i].view3_:refresh_gui(w, h)
        end
        
        self.planet_tab_.view_:refresh_gui(w, h)
        self.hidden = false
    end
end

function editor_tab_control:close_all()

    -- self:remove_page(self.planet_tab_)
    -- self.planet_tab_.page:hide()
    -- self.planet_tab_:hide()
    -- self.planet_tab_ = nil
    
    -- for i = 1, table.getn(self.sector_tabs_) do
        -- self.sector_tabs_[i].page:hide()
        -- self.sector_tabs_[i]:hide()
    -- end
    
    -- self.sector_tabs_ = {}
    -- self.hidden = true
end