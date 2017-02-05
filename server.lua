--[[
- @file server.lua
- @brief
]]

#include "common.h"

class 'server' (sf.Window)

function server:__init(w, h, title, style)
    sf.Window.__init(self, sf.VideoMode(w, h), title, style, sf.ContextSettings(0, 0, 0))
    self.event_ = sf.Event()
	self:set_position(0, 0)
	
	self.canvas_ = gwen.Canvas(w, h, "images/skin_dark.png")
	self.canvas_.renderer.render_to_fbo = true
	
	self.network_ = root.Network(root.Network.SERVER_TO_CLIENT)
	self.network_:set_auto_serialize_interval(100)
    self.camera_ = camera_first_person(math.radians(90), 1.0, 0.1, 15000000.0)
	self.offset_camera_ = root.Camera()
	self.dynamic_world_ = bt.DynamicWorld()
	self.players_ = {}
	
	self.temp_ = false
	
	root.connect(self.network_.on_connect, function()	
	
		self.planet_ = planet("sandbox/untitled_planet.json")
		self.planet_:parse()
		
		for i = 1, table.getn(self.planet_.faces_) do
			root.connect(self.planet_.faces_[i].on_page_in, function(tile)
			
			    if tile.level == tile.owner.max_level then
					self.dynamic_world_:add_terrain_collider(self.planet_.faces_[i].producer_, tile)				
					for i = 1, table.getn(self.planet_.active_sectors_) do
						local sector = self.planet_.active_sectors_[i]
						for j = 1, table.getn(sector.objects_) do
							sector.objects_[j]:on_physics_start(self.dynamic_world_)
						end
					end
				end
				
				if self.temp_ == false then
						
					-- temp
					local vehicule_object = car_object(self.planet_)
					table.insert(self.planet_.objects_, vehicule_object)		
					vehicule_object.transform_.position = vec3(50, 60, -195)
					vehicule_object:on_network_start(self.network_)	
					vehicule_object:on_physics_start(self.dynamic_world_)
					self.network_:reference(vehicule_object.replica_)					
					self.temp_ = true
				end
			end)
		end
	end)
	
	root.connect(self.network_.on_receive, function(packet)
	
		if packet.data[1] == MessageID.ID_PLAYER_INPUT then
		
			local bs = root.BitStream.build_from_packet(packet)
			local replica = self.network_:get_object_from_id(bs:read_network_id())
			replica.object_:receive_input(bs)
		end
	end)
	
	root.connect(self.network_.on_request, function(bs, network_id)
	
		local type_id = bs:read_integer()
		
		local object = player(self.planet_)
		table.insert(self.planet_.objects_, object)
		
		if type_id == ID_PLAYER_OBJECT then
			table.insert(self.players_, object)
		end
		
		object:on_network_start(self.network_)	
		object:on_physics_start(self.dynamic_world_)
		return object.replica_
	end)
	
	self.network_:connect("localhost", 5000)
end

function server:__update(dt)

    while self:poll_event(self.event_) do   
        if self.event_.type == sf.Event.Closed then
            self.owner:set_allocate(false)
			self.network_:shutdown(100)
            return
        end
		
		self.canvas_:process_event(self.event_)
    end
    
    self:set_active(true)
	
	for i = 1, table.getn(self.players_) do
	
		self.offset_camera_:copy(self.camera_)
		self.offset_camera_.position = self.players_[i]:get_camera_target() + vec3(0, 6360000.0, 0)
		self.offset_camera_:refresh()
		
		for j = 1, table.getn(self.planet_.faces_) do
			self.planet_.faces_[j]:cull(self.offset_camera_)
		end
	end
	
	self.canvas_:draw()
    self:display()
end

instance = server(800, 400, "Server", sf.Window.Default)