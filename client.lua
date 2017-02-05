--[[
- @file client.lua
- @brief
]]

#include "common.h"

class 'client' (sf.Window)

function client:__init(w, h, title, style)
    sf.Window.__init(self, sf.VideoMode(w, h), title, style, sf.ContextSettings(0, 0, 0))
    self.event_ = sf.Event()
	
	self.canvas_ = gwen.Canvas(w, h, "images/skin_dark.png")
	self.canvas_.renderer.render_to_fbo = true
	self.last_mouse_pos_ = vec2()
	
	self.network_ = root.Network(root.Network.CLIENT_TO_SERVER)
	self.network_:set_auto_serialize_interval(100)
	self.camera_ = camera_third_person(math.radians(90), w/h, 0.1, 15000000.0)
	self.offset_camera_ = root.Camera()
	self.active_player_ = nil
	
	root.connect(self.network_.on_connect, function()
	
		self.planet_ = planet("sandbox/untitled_planet.json")
		self.planet_:parse()
		
		self.render_pipeline_ = render_pipeline(self.planet_)
		self.render_pipeline_:rebuild(w, h)

		local bs = root.BitStream()
		bs:write_integer(ID_PLAYER_OBJECT)	
		
		self.player_id_ = self.network_:request(bs)
	end)
	
	root.connect(self.network_.on_request, function(bs, network_id)
	
		local type_id = bs:read_integer()
		local object = nil
		
		if type_id == ID_PLAYER_OBJECT then
			object = player(self.planet_)
		end
		
		-- temp
		if type_id == ID_CAR_OBJECT then
			object = car_object(self.planet_)
		end
		
		if self.player_id_ == network_id then
			self.active_player_ = object
		end
	
		table.insert(self.planet_.objects_, object)
		object:on_network_start(self.network_)
		return object.replica_
	end)
	
	self.network_:connect("localhost", 5000)
end

function client:process_view(dt)

    local mouse_x = sf.Mouse.get_position(self).x
    local mouse_y = sf.Mouse.get_position(self).y
    
    if self:has_focus() then
	
        if sf.Mouse.is_button_pressed(sf.Mouse.Right) then

            local delta = vec2()
            delta.x = mouse_x - self.last_mouse_pos_.x
            delta.y = mouse_y - self.last_mouse_pos_.y
            self.camera_:rotate(delta)
        end
		
		self.active_player_:process_input(self.camera_)
	end
	
	self.offset_camera_:copy(self.camera_)
	self.offset_camera_.position = self.offset_camera_.position + self.active_player_:get_camera_target() + vec3(0, 6360000.0, 0)
	self.offset_camera_:refresh()
		
	self.last_mouse_pos_ = vec2(mouse_x, mouse_y)
end

function client:__update(dt)

    while self:poll_event(self.event_) do 
        if self.event_.type == sf.Event.Closed then
            self.owner:set_allocate(false)
			self.network_:shutdown(100)
            return
        end
		
        if self.event_.type == sf.Event.Resized then
            self.render_pipeline_:rebuild(self.event_.size.w, self.event_.size.h)
            self.camera_.aspect = self.w/self.h
        end
		
		self.canvas_:process_event(self.event_)
    end
    
    self:set_active(true)
	gl.Viewport(0, 0, self.w, self.h)
	
	if self.active_player_ then
	
		self:process_view(dt)
		local final_tex = self.render_pipeline_:render(dt, self.offset_camera_)

		gl.ActiveTexture(gl.TEXTURE0)
		root.Shader.get():uniform("u_RenderingType", 2)
		root.Shader.get():sampler("s_Tex0", 0)
		final_tex:bind()
	
		self.render_pipeline_.screen_quad_:draw()
	end
	
	self.canvas_:draw()
    self:display()
end

instance = client(1280, 720, "Client", sf.Window.Default)