--[[
- @file editor.lua
- @brief
]]

#define WINDOW_TITLE "Planet Editor"

#include "common.h"
#include "editor_menu.h"

class 'editor' (sf.Window)

function editor:__init(w, h, title, style)
    sf.Window.__init(self, sf.VideoMode(w, h), title, style, sf.ContextSettings(0, 0, 0))
    self.clear_color_ = root.Color(50, 50, 50, 255)
    self.event_ = sf.Event()

    self.canvas_ = gwen.Canvas(w, h, "images/skin_dark.png")
    self.canvas_.renderer.render_to_fbo = true
    
    self.menu_strip_ = editor_menu(self, self.canvas_)          
    self.planet_ = planet()
end

function editor:create_new_planet()

    self.planet_:load(6360000.0, 14, 41223124)
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
        end
        
        self.canvas_:process_event(self.event_)
    end
    
    self:set_active(true)   
    gl.ClearColor(self.clear_color_)
    gl.Clear(gl.COLOR_BUFFER_BIT)
    
	gl.Viewport(0, 0, self.w, self.h)
    self.canvas_:draw()  
    self:display()
end

instance = editor(1000, 800, WINDOW_TITLE, sf.Window.Default)