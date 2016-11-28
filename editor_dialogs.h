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
// file_explorer implementation:
////////////////////////////////////////////////////////////////////////////////
]]

class 'file_explorer' (gwen.TreeControl)

function file_explorer:__init(parent, exts, cb)
	gwen.TreeControl.__init(self, parent)
    
    local function add_folder_node(parent, path, name)
    
        local node = parent:add_node(gwen.TreeNode(self), name)
		root.connect(node.on_select, function(control)
			cb(path)
		end)
        
        return node
    end
    
    local function add_handle_node(parent, path, name)
    
        local node = parent:add_node(gwen.TreeNode(self), name)
		root.connect(node.on_select, function(control)
			cb(path)
		end)
        
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
    
    self.file_explorer_ = file_explorer(self, exts, function(path)
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
		cb(absolute_path)
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
	
	self.file_explorer_ = file_explorer(self, exts, function(path)
    
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
    
	root.connect(self.ok_button_.on_press, function(control)
		cb("TEST")
	end)
end