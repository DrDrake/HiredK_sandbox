--[[
- @file editor_tab_control.h
- @brief
]]

class 'editor_tab_control' (gwen.TabControl)

function editor_tab_control:__init(editor, parent)
	gwen.TabControl.__init(self, parent)
    self.type = root.ScriptObject.Dynamic
	self.editor_ = editor
	
	self.tabs_count_ = 0
	self.tabs_focus_ = 1
	self.tabs_focus_prev_ = 1
	self.tabs_ = {}
end

function editor_tab_control:add_tab(name)

	local tab = self:add_page(gwen.TabButton(self), name)
	self.tabs_count_ = self.tabs_count_ + 1
	tab.index_ = self.tabs_count_
	table.insert(self.tabs_, tab)
	tab.views_ = {}
	
	root.connect(tab.on_press, function(control)
	
		if self.tabs_focus_ ~= control.index_ then
			self.tabs_focus_prev_ = self.tabs_focus_
			self.tabs_focus_ = control.index_
			self.editor_:refresh_all()
		end
	end)
	
	return tab
end

function editor_tab_control:add_planet_tab()

	for i = 1, table.getn(self.tabs_) do
		if self.tabs_[i].is_planet_ == true then
			self.tabs_[i]:press()
			return self.tabs_[i]
		end
	end
	
	local tab = self:add_tab(self.editor_.planet_.name)
	tab.views_[1] = editor_planet_view(self.editor_, tab.page)
	tab.views_[1]:dock(gwen.Base.Fill)
	tab.is_planet_ = true

	tab:press()
	return tab
end

function editor_tab_control:add_sector_tab(sector)

	for i = 1, table.getn(self.tabs_) do
		if self.tabs_[i].sector_ == sector then
			self.tabs_[i]:press()
			return self.tabs_[i]
		end
	end
	
	local tab = self:add_tab(sector.name)
	tab.splitter_ = gwen.CrossSplitter(tab.page)
	tab.splitter_:dock(gwen.Base.Fill)
	
	tab.views_[1] = editor_sector_first_person_view(self.editor_, tab.page, sector)
	tab.splitter_:set_panel(0, tab.views_[1])
	
	tab.views_[2] = editor_sector_ortho_view(self.editor_, tab.page, sector, math.NegY)
	tab.splitter_:set_panel(1, tab.views_[2])
	
	tab.is_planet_ = false
	tab.sector_ = sector
	
	root.connect(tab.on_right_press, function(control)
	
		control:press()
		self.editor_:create_dropdown_menu({ "Close" }, function(choice)				
			if choice == "Close" then		
				self:remove_tab(self:get_active_tab())
				collectgarbage()
			end
		end)
	end)
	
	tab:press()	
	return tab
end

function editor_tab_control:get_active_tab()

	for i = 1, table.getn(self.tabs_) do	
		if self.tabs_[i].index_ == self.tabs_focus_ then
			return self.tabs_[i]
		end
	end
	
	return nil
end

function editor_tab_control:refresh()

	self:hide()

	for i = 1, table.getn(self.tabs_) do
		self.tabs_[i].text = (self.tabs_[i].is_planet_) and self.editor_.planet_.name or self.tabs_[i].sector_.name
		self.tabs_[i]:size_to_contents()
		
		for j = 1, table.getn(self.tabs_[i].views_) do
			self.tabs_[i].views_[j]:__refresh()
		end
		
		self:show()
	end
end

function editor_tab_control:process_event(event)

	for i = 1, table.getn(self.tabs_) do
		if self.editor_:has_focus() and self.tabs_[i]:is_active() then
			for j = 1, table.getn(self.tabs_[i].views_) do
				self.tabs_[i].views_[j]:__process_event(event)
			end
			
			return
		end
	end
end

function editor_tab_control:__update(dt)

	for i = 1, table.getn(self.tabs_) do
		if self.editor_:has_focus() and self.tabs_[i]:is_active() then
			for j = 1, table.getn(self.tabs_[i].views_) do
				self.tabs_[i].views_[j]:__process(dt)
			end
			
			return
		end
	end
end

function editor_tab_control:remove_tab(tab)

	local tab_removed = false
	
	for i = 1, table.getn(self.tabs_) do
		if self.tabs_[i] == tab then
			self:remove_page(self.tabs_[i])
			table.remove(self.tabs_, i)
			tab_removed  = true
			collectgarbage()
			break
		end
	end
	
	if tab_removed then
		for i = 1, table.getn(self.tabs_) do
			if self.tabs_[i].index_ == self.tabs_focus_prev_ then
				self.tabs_[i]:press()
				break
			end
		end
	end
end

function editor_tab_control:remove_sector_tab(sector)

	for i = 1, table.getn(self.tabs_) do
		if not self.tabs_[i].is_planet_ and self.tabs_[i].sector_ == sector then
			self:remove_tab(self.tabs_[i])
			break
		end
	end
end

function editor_tab_control:clear()

	for i = 1, table.getn(self.tabs_) do
		self:remove_page(self.tabs_[i])
	end
	
	self.tabs_ = {}
end