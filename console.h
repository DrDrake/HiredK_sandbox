--[[
- @file console.h
- @brief
]]

class 'console' (gwen.WindowControl)

function console:__init(parent, w, h)
    gwen.WindowControl.__init(self, parent)
	self.type = root.ScriptObject.Dynamic
	
	self:set_title("Console")
	self:set_bounds(8, 8, w, h)
	self:set_closable(false)
	self:make_modal(true)
	self:hide()
	
	self.font_ = gwen.Font()
	self.font_.facename = "consola.ttf"
	self.font_.size = 10
	
	self.command_history_ = {}
	self.change_text_on_row_selected_ = true
	self.cursor_position_ = 0
	self.cursor_deferred_ = 0
	
	self.output_listbox_ = gwen.ListBox(self)
	self.output_listbox_:dock(gwen.Base.Fill)
	
	root.connect(LOG.on_received, function(level, message)	
		local row = self.output_listbox_:add_item(message)
		row:get_cell_contents(0).color = root.Color.White
		row:get_cell_contents(0):set_font(self.font_)
		self.output_listbox_:scroll_to_bottom()
	end)
	
	self.output_suggestions_listbox_ = gwen.ListBox(self.canvas)
	self.output_suggestions_listbox_:hide()
	
	self.output_suggestions_listbox_.refresh = function()
		self.output_suggestions_listbox_:clear()
		for suggestion in self.owner:check_command(self.input_textbox_.text).suggestions do
		
			local row = self.output_suggestions_listbox_:add_item(suggestion)	
			row:get_cell_contents(0).color = root.Color.White
			row:get_cell_contents(0):set_font(self.font_)
		end
	end
	
	self.bottom_area_ = gwen.ResizableControl(self)
	self.bottom_area_:dock(gwen.Base.Bottom)
	self.bottom_area_.h = 32
	
	self.input_textbox_ = gwen.TextBox(self.bottom_area_)
	self.input_textbox_:dock(gwen.Base.Fill)
	self.input_textbox_:set_font(self.font_)
	self.input_textbox_.margin_right = 10
	
	self.input_button_ = gwen.Button(self.bottom_area_)
	self.input_button_:dock(gwen.Base.Right)
	self.input_button_.disabled = true
	self.input_button_.text = "Send"
	self.input_button_.w = 64
	
	root.connect(self.output_suggestions_listbox_.on_row_selected, function(control)
		if self.change_text_on_row_selected_ then
			self.input_textbox_.text = control:get_selected_row():get_cell_contents(0).text
			self.input_textbox_:move_caret_to_end()
			self.input_textbox_:focus()
		end
	end)
	
	root.connect(self.input_textbox_.on_text_changed, function(control)
		self.input_button_.disabled = (self.input_textbox_.text_length == 0)
		self.output_suggestions_listbox_:refresh()
		self.cursor_position_ = 0
		self.cursor_deferred_ = 0
	end)
	
	root.connect(self.input_textbox_.on_return_press, function(control)
		if self.output_suggestions_listbox_:get_selected_row() then
			local current_row = self.output_suggestions_listbox_:get_selected_row()
			self.input_textbox_.text = current_row:get_cell_contents(0).text
			self.input_textbox_:move_caret_to_end()
			self.input_textbox_:focus()
		else
			self:input_command(self.input_textbox_.text)
		end
	end)
	
	root.connect(self.input_button_.on_down, function(control)
		self:input_command(self.input_textbox_.text)
	end)
end

function console:test()
	self:input_command("これはテストです")
end

function console:input_command(cmd)
	if self.owner:do_string(cmd) then
		table.insert(self.command_history_, cmd)
		self.input_textbox_.text = ""
	end
	
	self.output_suggestions_listbox_:bring_to_front()
	self.input_textbox_:focus()
end

function console:toggle()
	self.input_textbox_:move_caret_to_end()
	self.input_textbox_:focus()
	
	self.hidden = not self.hidden
end

function console:cursor_up()
	if not self.hidden and self.input_textbox_:has_focus() then
		if self.cursor_position_ <= 0 and math.abs(self.cursor_position_) < table.getn(self.command_history_) then
			self.cursor_position_ = self.cursor_position_ - 1
			
			self.input_textbox_:set_text(self.command_history_[(table.getn(self.command_history_) - math.abs(self.cursor_position_)) + 1], false)
			self.input_button_.disabled = false
			
			self.output_suggestions_listbox_:refresh()
			self.input_textbox_:move_caret_to_end()
			return
		end
		
		if self.cursor_position_ > 0 then
			self.cursor_position_ = self.cursor_position_ - 1
			
			self.change_text_on_row_selected_ = false
			local current_row = self.output_suggestions_listbox_.table:get_row(self.cursor_position_ - 1)
			self.output_suggestions_listbox_:set_selected_row(current_row, true)
			self.change_text_on_row_selected_ = true
			
			if self.cursor_deferred_ - 1 > 0 then
				self.cursor_deferred_ = self.cursor_deferred_ - 1
			else
				local scroll_amount = (self.cursor_position_ - 1) * (1.0/(self.output_suggestions_listbox_.table.num_children - 5))
				self.output_suggestions_listbox_:set_vertical_scrolled_amount(scroll_amount, true)
			end
		end
	end
end

function console:cursor_down()
	if not self.hidden and self.input_textbox_:has_focus() then	
	
		if self.cursor_position_ < 0 then
			self.cursor_position_ = self.cursor_position_ + 1
			
			if self.cursor_position_ ~= 0 then
				self.input_textbox_:set_text(self.command_history_[(table.getn(self.command_history_) - math.abs(self.cursor_position_)) + 1], false)
				self.input_button_.disabled = false
			else
				self.input_textbox_:set_text("", false)
				self.input_button_.disabled = true
			end
			
			self.output_suggestions_listbox_:refresh()
			self.input_textbox_:move_caret_to_end()
			return
		end
		
		if self.cursor_position_ >= 0 and self.cursor_position_ < self.output_suggestions_listbox_.table.num_children then
			self.cursor_position_ = self.cursor_position_ + 1
			
			self.change_text_on_row_selected_ = false
			local current_row = self.output_suggestions_listbox_.table:get_row(self.cursor_position_ - 1)
			self.output_suggestions_listbox_:set_selected_row(current_row, true)
			self.change_text_on_row_selected_ = true
			
			if self.cursor_deferred_ < 5 then
				self.cursor_deferred_ = self.cursor_deferred_ + 1
			else
				local scroll_amount = (self.cursor_position_ - 5) * (1.0/(self.output_suggestions_listbox_.table.num_children - 5))
				self.output_suggestions_listbox_:set_vertical_scrolled_amount(scroll_amount, true)
			end
		end
	end
end

function console:process_event(event)
	if event.type == sf.Event.TextEntered and
	   event.text.unicode == 96 then -- tilde
		self:toggle()
	end
	
	if event.type == sf.Event.KeyPressed then
		if event.key.code == sf.Keyboard.Up then
			self:cursor_up()
		end
		if event.key.code == sf.Keyboard.Down then
			self:cursor_down()
		end
	end
end

function console:__update(dt)
	if self.input_textbox_:has_focus() then
		if self.output_suggestions_listbox_.hidden then
			local pos = self:local_pos_to_canvas(gwen.Point(0, self.h))
			self.output_suggestions_listbox_:set_bounds(pos.x + 10, pos.y - 10, self.output_listbox_.w - self.input_button_.w - 16, 80)
			self.output_suggestions_listbox_:scroll_to_top()
			self.output_suggestions_listbox_:refresh()
			self.output_suggestions_listbox_:show()
		end
		
		self.output_suggestions_listbox_:bring_to_front()
	else
		self.output_suggestions_listbox_:hide()
		if self.cursor_position_ > 0 then
			self.cursor_position_ = 0
			self.cursor_deferred_ = 0
		end
	end
end