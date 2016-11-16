--[[
- @file editor.lua
- @brief
]]

#include "shared/camera_first_person.h"
#include "shared/camera_orbital.h"
#include "shared/planet.h"
#include "shared/planet_atmosphere.h"
#include "shared/planet_clouds.h"
#include "shared/planet_ocean.h"
#include "shared/planet_producer.h"
#include "shared/planet_sector.h"
#include "shared/render_pipeline.h"
#include "shared/scene.h"
#include "shared/sun.h"

#include "editor_dialogs.h"
#include "editor_menu.h"
#include "editor_tab_control.h"
#include "editor_toolbox.h"
#include "editor_view.h"

class 'editor' (sf.Window)

function editor:__init(w, h, title, style)
    sf.Window.__init(self, sf.VideoMode(w, h), title, style, sf.ContextSettings(0, 0, 0))
    self.clear_color_ = root.Color(50, 50, 50, 255)
    self.event_ = sf.Event()
    
    self.client_entry_ = FileSystem:search("client.lua")
    
    self.canvas_ = gwen.Canvas(w, h, "skin_dark.png")
    self.canvas_.renderer.render_to_fbo = true
    self.scene_ = scene()
    
    self.menu_strip_ = editor_menu(self, self.canvas_)  
	self.dock_base_ = gwen.DockBase(self.canvas_)
	self.dock_base_:dock(gwen.Base.Fill)
    
    self.tab_control_ = editor_tab_control(self, self.dock_base_)
    self.tab_control_:dock(gwen.Base.Fill)
    
	self.toolbox_ = editor_toolbox(self, self.dock_base_)
    self.dock_base_:get_left():get_tab_control():add_page("Toolbox", self.toolbox_)
    
    self.status_bar_ = gwen.StatusBar(self.canvas_)
	self.status_bar_.h = 24
    self:refresh_gui()
end

function editor:create_new_planet()

	if self.new_planet_dialog_ == nil or self.new_planet_dialog_.hidden then
		self.new_planet_dialog_ = new_planet_dialog(self, self.canvas_, function(name)
        
            self.scene_:load_planet(6360000.0, 14, true)	       
            self:refresh_gui(self.w, self.h)

			self.new_planet_dialog_:close_button_pressed()
			self.new_planet_dialog_ = nil
		end)
	end
end

function editor:refresh_gui()

    self.tab_control_:refresh_gui(self.w, self.h)
    self.toolbox_:refresh_gui(self.w, self.h)
end

function editor:__update(dt)

    while self:poll_event(self.event_) do
        if self.event_.type == sf.Event.Closed then
            Engine:shutdown()
            return
        end
        
        if self.event_.type == sf.Event.Resized then
            self.canvas_.w = self.event_.size.w
            self.canvas_.h = self.event_.size.h
            self:refresh_gui()
        end
        
        self.tab_control_:process_event(self.event_)
        self.canvas_:process_event(self.event_)
    end
    
    self:set_active(true)
    
    gl.ClearColor(self.clear_color_)
    gl.Clear(gl.COLOR_BUFFER_BIT)
	gl.Viewport(0, 0, self.w, self.h)
    self.canvas_:draw()
	
    self:display()
end

instance = editor(1000, 800, "Planet Editor", sf.Window.Default)