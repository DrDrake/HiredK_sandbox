--[[
- @file main.lua
- @brief
]]

#include "console.h"
#include "first_person_camera.h"
#include "post_fx.h"
#include "scene.h"

class 'client' (sf.Window)

function client:__init(w, h, title, style)
    sf.Window.__init(self, sf.VideoMode(w, h), title, style, sf.ContextSettings(0, 0, 0))
    self.clear_color_ = root.Color(50, 50, 50, 255)
    self.event_ = sf.Event()
	
	self:set_vertical_sync_enabled(true)
	
    self.canvas_ = gwen.Canvas(w, h, "skin_dark.png")
    self.canvas_.renderer.render_to_fbo = true	
    self.console_ = console(self.canvas_, 500, 500)
	
    self.camera_ = first_person_camera(math.radians(90), w/h, 0.1, 5000000.0)
	self.post_fx_ = post_fx()
	self.scene_ = scene()
	
	self:rebuild_display()
end

function client:rebuild_display()

    self.gbuffer_depth_tex_ = root.Texture.from_empty(self.w, self.h, { iformat = gl.DEPTH_COMPONENT32F, format = gl.DEPTH_COMPONENT, type = gl.FLOAT })
    self.gbuffer_diffuse_tex_ = root.Texture.from_empty(self.w, self.h)
	self.gbuffer_geometric_tex_ = root.Texture.from_empty(self.w, self.h, { iformat = gl.RGBA32F, type = gl.FLOAT, filter_mode = gl.LINEAR })
	self.gbuffer_fbo_ = root.FrameBuffer()
	
	self.post_fx_:rebuild(self.w, self.h)
	root.FrameBuffer.unbind()
end

function client:__update(dt)

    while self:poll_event(self.event_) do
        if self.event_.type == sf.Event.Closed then
            Engine:shutdown()
            return
        end
		
        if self.event_.type == sf.Event.Resized then
            self.canvas_.w = self.event_.size.w
            self.canvas_.h = self.event_.size.h
			self:rebuild_display()
        end
        
        self.canvas_:process_event(self.event_)
        self.console_:process_event(self.event_)
    end
	
	self.scene_:make_shadow(self.camera_)
	
	self.gbuffer_fbo_:clear()
	self.gbuffer_fbo_:attach(self.gbuffer_depth_tex_, gl.DEPTH_ATTACHMENT)
	self.gbuffer_fbo_:attach(self.gbuffer_diffuse_tex_, gl.COLOR_ATTACHMENT0)
	self.gbuffer_fbo_:attach(self.gbuffer_geometric_tex_, gl.COLOR_ATTACHMENT1)
	self.gbuffer_fbo_:bind_output()
	 
    gl.ClearColor(root.Color.Transparent)
    gl.Clear(bit.bor(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT))
    gl.Viewport(0, 0, self.w, self.h)
	
	self.scene_:draw(self.camera_)	
	root.FrameBuffer.unbind()
	
	self.post_fx_:process(dt, self.camera_, self.scene_, self.gbuffer_depth_tex_, self.gbuffer_diffuse_tex_, self.gbuffer_geometric_tex_)
	
	gl.ActiveTexture(gl.TEXTURE0) -- temp
	self.canvas_:draw()	
    self:display()
end

instance = client(1280, 720, "Client", sf.Window.Default)