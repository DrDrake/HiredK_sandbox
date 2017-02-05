--[[
- @file editor_toolbox.h
- @brief
]]

class 'editor_toolbox' (gwen.Base)

function editor_toolbox:__init(editor, parent)
	gwen.Base.__init(self, parent)
	self.editor_ = editor
	
	self.tab_control_ = gwen.TabControl(self)
	self.tab_control_:dock(gwen.Base.Fill)
	
	self.objects_tab_ = self.tab_control_:add_page(gwen.TabButton(self), "Objects")
	self.objects_tab_.page_ = editor_toolbox_objects(editor, self.objects_tab_.page)
	self.objects_tab_.page_:dock(gwen.Base.Fill)
	self.created_objects_count_ = 0
	
	self.properties_tab_ = self.tab_control_:add_page(gwen.TabButton(self), "Properties")
	self.properties_tab_.page_ = editor_toolbox_properties(editor, self.properties_tab_.page)
	self.properties_tab_.page_:dock(gwen.Base.Fill)
	
    self.bottom_control_ = gwen.ResizableControl(self)
	self.bottom_control_:dock(gwen.Base.Bottom)
 	self.bottom_control_.margin_right = -6
	self.bottom_control_.margin_left = -6
	self.bottom_control_.margin_bottom = -6
	self.bottom_control_.h = 300
    
	self.planet_tree_control_ = gwen.TreeControl(self.bottom_control_)
	self.planet_tree_control_:allow_multiselect(true)
	self.planet_tree_control_:dock(gwen.Base.Fill)
	self.nodes_ = {}
end

function editor_toolbox:process_event(event)

	if event.type == sf.Event.KeyPressed and event.key.code == sf.Keyboard.Delete then
		self.editor_.selection_:delete_selected_objects()
	end

	self.objects_tab_.page_:process_event(event)
end

function editor_toolbox:refresh()

	self.planet_tree_control_:clear()
	self.nodes_ = {}

	if self.editor_:is_planet_loaded() then

		local planet_node = self.editor_:add_node_atlas(self.planet_tree_control_, self.editor_.planet_.name, 1, 0)
		table.insert(self.nodes_, planet_node)
		
        root.connect(planet_node.on_double_click, function(control)
			self.editor_.tab_control_:add_planet_tab()
			self.nodes_[1]:open()
        end)
		
		root.connect(planet_node.on_select, function(control)
			if sf.Keyboard.is_key_pressed(sf.Keyboard.LControl) then
				self.nodes_[1]:set_selected(false, false)
			end
		end)
		
		root.connect(planet_node.on_right_press, function(control)
			self.editor_:create_dropdown_menu({ "New Sector", "Rename" }, function(choice)
				if choice == "New Sector" then
					self.editor_:create_new_sector()
				end
				if choice == "Rename" then
					self.editor_:create_textbox_edit(self.nodes_[1], function(input)
						self.editor_.planet_.name = input
						self.editor_:refresh_all()
					end)
				end
			end)
		end)
		
		for i = 1, table.getn(self.editor_.planet_.loaded_sectors_) do
		
			local sector = self.editor_.planet_.loaded_sectors_[i]
			local sector_node = self.editor_:add_node_atlas(planet_node, sector.name, 8, 0)
			table.insert(self.nodes_, sector_node)
			sector_node.nodes_ = {}
			
			root.connect(sector_node.on_double_click, function(control)
				local sector = self.editor_.planet_.loaded_sectors_[i]
				local sector_tab = self.editor_.tab_control_:add_sector_tab(sector)
				sector_tab.sector_:parse()
				self.editor_:refresh_all()
			end)
			
			root.connect(sector_node.on_select, function(control)
				if sf.Keyboard.is_key_pressed(sf.Keyboard.LControl) then
					self.nodes_[i]:set_selected(false, false)
				end
			end)
			
			root.connect(sector_node.on_right_press, function(control)		
				self.editor_:create_dropdown_menu({ "Delete", "Rename" }, function(choice)
				
					if choice == "Delete" then
						local sector = self.editor_.planet_.loaded_sectors_[i]
						self.editor_:delete_sector(sector)
					end
					if choice == "Rename" then
						local sector = self.editor_.planet_.loaded_sectors_[i]
						self.editor_:create_textbox_edit(self.nodes_[1 + i], function(input)
							sector.name = input
							self.editor_:refresh_all()
						end)
					end
				end)
			end)
			
			for j = 1, table.getn(sector.objects_) do
			
				local object = sector.objects_[j]
				local object_node = self.editor_:add_node_atlas(sector_node, object.name, 8, 0)
				object_node:set_selected(self.editor_.selection_:is_selected(object), false)
				table.insert(sector_node.nodes_, object_node)
				
				root.connect(object_node.on_select, function(control)
					if not sf.Keyboard.is_key_pressed(sf.Keyboard.LControl) then
						self.editor_.selection_:clear()
					end
					
					local object = self.editor_.planet_.loaded_sectors_[i].objects_[j]
					self.editor_.selection_:add_to_selected(object)
				end)
				
				root.connect(object_node.on_unselect, function(control)
					local object = self.editor_.planet_.loaded_sectors_[i].objects_[j]
					self.editor_.selection_:remove_selected(object)
				end)
				
				root.connect(object_node.on_right_press, function(control)
					local choices = (not self.editor_.selection_:is_multiselect()) and { "Delete", "Rename" } or { "Delete Selected" }
					self.editor_:create_dropdown_menu(choices, function(choice)
						if choice == "Delete" or choice == "Delete Selected" then
							self.editor_.selection_:delete_selected_objects()
						end				
						if choice == "Rename" then
							self.editor_:create_textbox_edit(self.nodes_[1 + i].nodes_[j], function(input)
								local object = self.editor_.planet_.loaded_sectors_[i].objects_[j]
								object.name = input
								self.editor_:refresh_all()
							end)
						end
					end)
				end)
				
				sector_node:open()
			end
			
			planet_node:open()
		end
	end
end