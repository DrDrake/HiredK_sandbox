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