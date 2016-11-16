--[[
- @file client.lua
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

class 'client' (sf.Window)

function client:__init(w, h, title, style)
    sf.Window.__init(self, sf.VideoMode(w, h), title, style, sf.ContextSettings(0, 0, 0))
    self.event_ = sf.Event()
    
    --self:set_vertical_sync_enabled(true)
    self:set_framerate_limit(60)
    
    self.canvas_ = gwen.Canvas(w, h, "skin_dark.png")
    self.canvas_.renderer.render_to_fbo = true
    
    self.dynamic_world_ = bt.DynamicWorld()
    self.player_ = bt.CharacterController(self.dynamic_world_, 0.8, 2.0)
    self.player_:wrap(vec3(0, 10, 0))
    
    self.scene_ = scene()
    local planet = self.scene_:load_planet(6360000.0, 14, false)
    planet.clouds_:load()
    planet.ocean_:load()
    
    self.sector_camera_ = camera_first_person(math.radians(90), w/h, 0.1, 15000000.0)
    self.planet_camera_ = root.Camera()
    self.render_pipeline_ = render_pipeline(self.scene_)
    self.render_pipeline_:rebuild(w, h)
    self.last_mouse_pos_ = vec2()
    
    for i = 1, table.getn(self.scene_.planet_.faces_) do   
        root.connect(self.scene_.planet_.faces_[i].on_page_in, function(tile)
            if tile.level == tile.owner.max_level then
                self.dynamic_world_:add_terrain_collider(self.scene_.planet_.faces_[i].producer_, tile)
            end
        end)
    end
end

function client:process_view(dt)

    local mouse_x = sf.Mouse.get_position(self).x
    local mouse_y = sf.Mouse.get_position(self).y
    
    if self:has_focus() then
        if sf.Mouse.is_button_pressed(sf.Mouse.Right) then
        
            local delta = vec2()
            delta.x = mouse_x - self.last_mouse_pos_.x
            delta.y = mouse_y - self.last_mouse_pos_.y
            self.sector_camera_:rotate(delta)
        end
        
        local speed = (sf.Keyboard.is_key_pressed(sf.Keyboard.LShift) and 8.0 or 0.15)
        local direction = vec3()
        
        if sf.Keyboard.is_key_pressed(sf.Keyboard.W) then
            direction = direction - vec3(math.column(self.sector_camera_.local_, 2)) * speed
        end		
        if sf.Keyboard.is_key_pressed(sf.Keyboard.S) then
            direction = direction + vec3(math.column(self.sector_camera_.local_, 2)) * speed
        end
        if sf.Keyboard.is_key_pressed(sf.Keyboard.D) then
            direction = direction + vec3(math.column(self.sector_camera_.local_, 0)) * speed
        end
        if sf.Keyboard.is_key_pressed(sf.Keyboard.A) then
            direction = direction - vec3(math.column(self.sector_camera_.local_, 0)) * speed
        end
        
        if sf.Keyboard.is_key_pressed(sf.Keyboard.Space) then
            self.player_:jump(10.0)
        end
        
        self.player_:set_walk_direction(direction)
    end
    
	self.sector_camera_.position = self.player_.position + vec3(0.0, 0.8, 0.0)
    self.sector_camera_:refresh()
    
	self.planet_camera_:copy(self.sector_camera_)
    self.planet_camera_.position = self.sector_camera_.position + self.scene_.planet_.loaded_sectors_[3].position_
	self.planet_camera_:refresh()
    
    self.last_mouse_pos_ = vec2(mouse_x, mouse_y)
end

function client:__update(dt)

    while self:poll_event(self.event_) do
        if self.event_.type == sf.Event.Closed then
            self.owner:set_allocate(false)
            return
        end
        
        if self.event_.type == sf.Event.Resized then
            self.render_pipeline_:rebuild(self.event_.size.w, self.event_.size.h)
        end
    end
    
    self:set_active(true)
    self:process_view(dt)
    
    gl.Viewport(0, 0, self.w, self.h)   
    local final_tex = self.render_pipeline_:render(self.planet_camera_, self.sector_camera_)
    
    gl.ActiveTexture(gl.TEXTURE0)
    root.Shader.get():uniform("u_RenderingType", 2)
    root.Shader.get():sampler("s_Tex0", 0)
    final_tex:bind()
    
    self.render_pipeline_.screen_quad_:draw()
    self.canvas_:draw()
    self:display()
end

instance = client(1280, 720, "Client", sf.Window.Default)