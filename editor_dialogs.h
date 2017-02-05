--[[
- @file editor_dialogs.h
- @brief
]]

DEFAULT_DIALOG_W = 600
DEFAULT_DIALOG_H = 400

--[[
////////////////////////////////////////////////////////////////////////////////
// dialog_base implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'dialog_base' (gwen.WindowControl)

function dialog_base:__init(editor, parent)
	gwen.WindowControl.__init(self, parent)
	self.editor_ = editor
	
	self.w = DEFAULT_DIALOG_W
	self.h = DEFAULT_DIALOG_H
    
	self.ok_button_ = gwen.Button(self)
	self.ok_button_:set_text("OK")
	self.ok_button_.w = 120
	self.ok_button_.x = self.w - self.ok_button_.w - 25
	self.ok_button_.y = 10
    
	self.cancel_button_ = gwen.Button(self)
	self.cancel_button_:set_text("Cancel")
	self.cancel_button_.w = 120
	self.cancel_button_.x = self.w - self.ok_button_.w - 25
	self.cancel_button_.y = self.ok_button_.y + self.ok_button_.h + 10
	
	root.connect(self.cancel_button_.on_press, function(control)
		self:close_button_pressed()
	end)
	
	self:position(gwen.Base.Center)
	self:disable_resizing()
end

--[[
////////////////////////////////////////////////////////////////////////////////
// confirm_dialog implementation:
////////////////////////////////////////////////////////////////////////////////
]]

#define CONFIRM_DIALOG_W 400
#define CONFIRM_DIALOG_H 150

class 'confirm_dialog' (gwen.WindowControl)

function confirm_dialog:__init(editor, parent, title, text, cb)
	gwen.WindowControl.__init(self, parent)
	self.editor_ = editor
	
	self.w = CONFIRM_DIALOG_W
	self.h = CONFIRM_DIALOG_H
	self:position(gwen.Base.Center)
	self:disable_resizing()
	
	self:set_title(title)
	self:set_closable(false)
	self:make_modal()
	
	self.image_panel_ = gwen.ImagePanel(self)	
	self.image_panel_:set_image("images/exclam.png")
	self.image_panel_:set_size(48, 48)
	self.image_panel_.x = self.image_panel_.w * 0.5
	self.image_panel_.y = self.image_panel_.h * 0.5
	
	self.text_label_ = gwen.Label(self)
	self.text_label_:set_text(text)
	self.text_label_:set_wrap(true)
	self.text_label_.x = 100
	self.text_label_.y = 20
	self.text_label_.w = self.w - self.text_label_.x - 25
	self.text_label_.h = self.h - self.text_label_.y
	
	self.yes_button_ = gwen.Button(self)
	self.yes_button_:set_text("Yes")
	self.yes_button_.w = 60
	self.yes_button_.x = self.w - (self.yes_button_.w * 2) - 35
	self.yes_button_.y = 85
	
	root.connect(self.yes_button_.on_press, function(control)
		cb()
	end)
	
	self.no_button_ = gwen.Button(self)
	self.no_button_:set_text("No")
	self.no_button_.w = 60
	self.no_button_.x = self.w - self.no_button_.w - 25
	self.no_button_.y = 85
	
	root.connect(self.no_button_.on_press, function(control)
		self:close_button_pressed()
	end)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// file_explorer implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'file_explorer' (gwen.TreeControl)

function file_explorer:__init(editor, parent, exts, cb)
	gwen.TreeControl.__init(self, parent)
	self.editor_ = editor
	self.nodes_ = {}
    
    local function add_folder_node(parent, path, name)
    
        local node = self.editor_:add_node_atlas(parent, name, 4, 0)
		root.connect(node.on_select, function(control)
			cb(path)
		end)
        
		table.insert(self.nodes_, node)
        return node
    end
    
    local function add_handle_node(parent, path, name)
    
        local node = self.editor_:add_node_atlas(parent, name, 5, 0)
		root.connect(node.on_select, function(control)
			cb(path)
		end)
        
		table.insert(self.nodes_, node)
        return node
    end
    
    local function discover_group(node, path)
    
        for k, value in pairs(root.FileSystem.enumerate_files(path)) do
        
            local absolute_path = path .. "/" .. value
            
            if root.FileSystem.is_directory(absolute_path) then
                discover_group(add_folder_node(node, absolute_path .. "/", value), absolute_path)
            else           
                for i = 1, table.getn(exts) do
                    if string.upper(root.FileSystem.extract_ext(value)) == string.upper(exts[i]) then
                        add_handle_node(node, absolute_path, value)
                        break
                    end
                end
            end
        end
    end
    
    for i = 1, FileSystem.number_of_groups do
    
        local group_path = FileSystem:get_group(i - 1).path
        local group_node = add_folder_node(self, group_path .. "/", group_path)
        discover_group(group_node, group_path)
        
        group_node:set_selected(true, true)
        group_node:open()
    end
end

--[[
////////////////////////////////////////////////////////////////////////////////
// save_file_dialog implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'save_file_dialog' (dialog_base)

function save_file_dialog:__init(editor, parent, title, exts, default_name, cb)
	dialog_base.__init(self, editor, parent)
	
	self:set_title(title)    
	self.ok_button_:set_text("Save")
    self.selected_path_ = ""
	
	self.name_label_ = gwen.Label(self)
	self.name_label_:set_text("Name :")
	self.name_label_.x = 75
	self.name_label_.y = 15
    
    self.name_textbox_ = gwen.TextBox(self)
    self.name_textbox_:set_text(default_name, true)
	self.name_textbox_.x = 120
	self.name_textbox_.y = 10
	self.name_textbox_.w = (self.w * 0.5) + 20
	self.name_textbox_:move_caret_to_end()
	self.name_textbox_:focus()
    
    self.file_explorer_ = file_explorer(editor, self, exts, function(path)
		local dir, name, ext = string.match(path, "(.-)([^\\/]-%.?([^%.\\/]*))$")
		if name ~= "" then
			self.name_textbox_:set_text(name, true)
		end
		
		self.selected_path_ = dir
    end)
    
	self.file_explorer_.x = 10
	self.file_explorer_.y = 40
	self.file_explorer_.w = self.w - 170
	self.file_explorer_.h = self.h - 85
    
	root.connect(self.ok_button_.on_press, function(control)
        local absolute_path = self.selected_path_ .. self.name_textbox_.text
		if root.FileSystem.exists(absolute_path) then	
			local text = string.format("The file '%s' already exists. Do you want to replace it?", absolute_path)
			self.editor_:create_confirm_dialog("Confirm Save As", text, function()
				cb(absolute_path)
			end)
		else
			cb(absolute_path)
		end
	end)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// open_file_dialog implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'open_file_dialog' (dialog_base)

function open_file_dialog:__init(editor, parent, title, exts, cb)
	dialog_base.__init(self, editor, parent)
	
	self:set_title(title)
	self.ok_button_:set_text("Open")
	self.selected_path_ = ""
	
	self.file_explorer_ = file_explorer(editor, self, exts, function(path)   
        local dir, name, ext = string.match(path, "(.-)([^\\/]-%.?([^%.\\/]*))$")    
        self.selected_path_ = path
	end)
	
	self.file_explorer_.x = 10
	self.file_explorer_.y = 10
	self.file_explorer_.w = self.w - 170
	self.file_explorer_.h = self.h - 55
	
	root.connect(self.ok_button_.on_press, function(control)
        cb(self.selected_path_)
	end)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// new_planet_dialog implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'new_planet_dialog' (dialog_base)

function new_planet_dialog:__init(editor, parent, cb)
	dialog_base.__init(self, editor, parent)
	
	self:set_title("New Planet")
	self:make_modal()
	
	self.name_label_ = gwen.Label(self)
	self.name_label_:set_text("Name :")
	self.name_label_.x = 75
	self.name_label_.y = 15
	
	self.name_textbox_ = gwen.TextBox(self)
	self.name_textbox_:set_text("Untitled Planet")
	self.name_textbox_.x = 120
	self.name_textbox_.y = 10
	self.name_textbox_.w = (self.w * 0.5) + 20
	self.name_textbox_:move_caret_to_end()
	self.name_textbox_:focus()
	
	root.connect(self.name_textbox_.on_text_changed, function(control)
		self.ok_button_:set_disabled(self.name_textbox_.text_length == 0 and true or false)
	end)
	
	root.connect(self.name_textbox_.on_return_press, function(control)
		if not self.ok_button_.disabled then self.ok_button_:press() end
		self.name_textbox_:focus()
	end)
	
	self.properties_gb_ = gwen.GroupBox(self)
	self.properties_gb_:set_text("Properties :")
	self.properties_gb_.x = 10
	self.properties_gb_.y = 35
	self.properties_gb_.w = self.w - 170
	self.properties_gb_.h = self.h - 80
	
	self.radius_label_ = gwen.Label(self.properties_gb_)
	self.radius_label_:set_text("Radius :")
	self.radius_label_.x = 60
	self.radius_label_.y = 15
	
	self.radius_slider_ = gwen.HorizontalSlider(self.properties_gb_)
	self.radius_slider_:set_size(225, 24)
	self.radius_slider_:set_range(10, 10000)
	self.radius_slider_:set_value(6360) -- default earth radius in km
	self.radius_slider_:set_clamp_to_notches(true)
	self.radius_slider_:set_notch_count(50)
	self.radius_slider_.x = 100
	self.radius_slider_.y = 10
	
	self.radius_textbox_ = gwen.TextBoxNumeric(self.properties_gb_)
	self.radius_textbox_:set_size(56, 18)
	self.radius_textbox_.text = tostring(self.radius_slider_.value)
	self.radius_textbox_.x = 325
	self.radius_textbox_.y = 12
	
	self.radius_unit_label_ = gwen.Label(self.properties_gb_)
	self.radius_unit_label_:set_text("(km)")
	self.radius_unit_label_.x = 390
	self.radius_unit_label_.y = 15
	
	root.connect(self.radius_slider_.on_value_changed, function(control)
		self.radius_textbox_.text = tostring(math.floor(control.value))
	end)
	
	self.seed_label_ = gwen.Label(self.properties_gb_)
	self.seed_label_:set_text("Seed :")
	self.seed_label_.x = 60
	self.seed_label_.y = 55
	
	self.seed_textbox_ = gwen.TextBoxNumeric(self.properties_gb_)
	self.seed_textbox_.x = 105
	self.seed_textbox_.y = 50
	self.seed_textbox_.w = 275
	
	math.randomseed(os.time())
	self.seed_textbox_.text = tostring(math.floor(100000 + math.random() * 900000))
    
	root.connect(self.ok_button_.on_press, function(control)
		local radius = self.radius_textbox_:get_float_from_text() * 1000 -- send in meters
		local seed = self.seed_textbox_:get_float_from_text()
		cb(self.name_textbox_.text, radius, seed)
	end)
end

--[[
////////////////////////////////////////////////////////////////////////////////
// vec3_edit_control implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'vec3_edit_control' (gwen.Base)

function vec3_edit_control:__init(parent, name)
	gwen.Base.__init(self, parent)
	self:set_size(350, 24)
	
	self.name_label_ = gwen.Label(self)
	self.name_label_:set_text(name)
	self.name_label_.x = 0
	self.name_label_.y = 5
	
	-- x component
	self.x_label_ = gwen.Label(self)
	self.x_label_:set_text("X")
	self.x_label_.x = 45
	self.x_label_.y = 5		
	self.x_textbox_ = gwen.TextBoxNumeric(self)
	self.x_textbox_.x = 60
	self.x_textbox_.y = 0
	self.x_textbox_.w = 80
	
	-- y component
	self.y_label_ = gwen.Label(self)
	self.y_label_:set_text("Y")
	self.y_label_.x = 150
	self.y_label_.y = 5	
	self.y_textbox_ = gwen.TextBoxNumeric(self)
	self.y_textbox_.x = 165
	self.y_textbox_.y = 0
	self.y_textbox_.w = 80
	
	-- z component
	self.z_label_ = gwen.Label(self)
	self.z_label_:set_text("Z")
	self.z_label_.x = 255
	self.z_label_.y = 5	
	self.z_textbox_ = gwen.TextBoxNumeric(self)
	self.z_textbox_.x = 270
	self.z_textbox_.y = 0
	self.z_textbox_.w = 80
end

function vec3_edit_control:get_value()
	
	local result = vec3()
	result.x = self.x_textbox_:get_float_from_text()
	result.y = self.y_textbox_:get_float_from_text()
	result.z = self.z_textbox_:get_float_from_text()
	return result
end

--[[
////////////////////////////////////////////////////////////////////////////////
// new_sector_dialog implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'new_sector_dialog' (dialog_base)

function new_sector_dialog:__init(editor, parent, cb)
	dialog_base.__init(self, editor, parent)
	
	self:set_title("New Sector")
	self:make_modal()
	
	self.name_label_ = gwen.Label(self)
	self.name_label_:set_text("Name :")
	self.name_label_.x = 75
	self.name_label_.y = 15
	
	self.name_textbox_ = gwen.TextBox(self)
	self.name_textbox_:set_text("Untitled Sector")
	self.name_textbox_.x = 120
	self.name_textbox_.y = 10
	self.name_textbox_.w = (self.w * 0.5) + 20
	self.name_textbox_:move_caret_to_end()
	self.name_textbox_:focus()
	
	self.properties_gb_ = gwen.GroupBox(self)
	self.properties_gb_:set_text("Properties :")
	self.properties_gb_.x = 10
	self.properties_gb_.y = 35
	self.properties_gb_.w = self.w - 170
	self.properties_gb_.h = self.h - 80
	
	self.coord_vec3_edit_ = vec3_edit_control(self.properties_gb_, "Coord :")
	self.coord_vec3_edit_.x = 60
	self.coord_vec3_edit_.y = 10
	
	self.face_label_ = gwen.Label(self.properties_gb_)
	self.face_label_:set_text("Face :")
	self.face_label_.x = 60
	self.face_label_.y = 55
	
	self.faces_cb_ = gwen.ComboBox(self.properties_gb_)
	self.faces_cb_.x = 102
	self.faces_cb_.y = 48
	self.faces_cb_:set_size(98, 24)
	self.face_selection_ = 1
	self.faces_items_ = {}
	
	for i = 1, 6 do
	
		local face_name = ""
		if i == 1 then face_name = "Right" end
		if i == 2 then face_name = "Left" end
		if i == 3 then face_name = "Top" end
		if i == 4 then face_name = "Bottom" end
		if i == 5 then face_name = "Front" end
		if i == 6 then face_name = "Back" end
		
		local item = self.faces_cb_:add_item(gwen.MenuItem(self), face_name)
		table.insert(self.faces_items_, item)
		
		root.connect(item.on_menu_item_selected, function(control)
			self.face_selection_ = i
		end)
	end
	
	root.connect(self.ok_button_.on_press, function(control)
		cb(self.name_textbox_.text, self.coord_vec3_edit_:get_value(), self.face_selection_)
	end)
end