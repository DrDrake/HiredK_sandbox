--[[
- @file client.lua
- @brief
]]

#include "shared/camera_first_person.h"
#include "shared/camera_orbital.h"
#include "shared/camera_ortho.h"
#include "shared/object.h"
#include "shared/object_common.h"
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
    self.player_ = bt.CharacterController(self.dynamic_world_, 1.0, 0.1)
    self.player_:wrap(vec3(0, 10, 0))
    
    self.picking_constraint_ = nil
    self.picked_body_ = nil
    self.old_picking_pos_ = vec3()
    self.hit_pos_ = vec3()
    self.old_picking_dist_ = 0.0
    self.saved_state_ = 0
    
    self.scene_ = scene()
    local planet = self.scene_:load_planet("sandbox/test.json")
    planet:parse()
    
    planet.clouds_:load()
    planet.ocean_:load()
    
    local sector = self.scene_.planet_.loaded_sectors_[1]
    for i = 1, table.getn(sector.objects_) do
        sector.objects_[i]:on_physics_start(self.dynamic_world_)
    end
    
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

function client:screen_to_world_ray(x, y)

    local ray_start = vec4(0, 0, -1, 1)
    ray_start.x = (x / self.w - 0.5) * 2.0
    ray_start.y = (y / self.h - 0.5) * 2.0
    
    local ray_end = vec4(0, 0, 0, 1)
    ray_end.x = (x / self.w - 0.5) * 2.0
    ray_end.y = (y / self.h - 0.5) * 2.0
    
    local inv_proj = math.inverse(self.sector_camera_.proj_matrix)
    local inv_view = math.inverse(self.sector_camera_.view_matrix)
    
    local ray_start_vs = inv_proj * ray_start
    ray_start_vs = ray_start_vs / ray_start_vs.w   
    local ray_start_ws = inv_view * ray_start_vs
    ray_start_ws = ray_start_ws / ray_start_ws.w
    
    local ray_end_vs = inv_proj * ray_end
    ray_end_vs = ray_end_vs / ray_end_vs.w   
    local ray_end_ws = inv_view * ray_end_vs
    ray_end_ws = ray_end_ws / ray_end_ws.w
    
    local ray = {}
    ray.origin = vec3(ray_start_ws)
    ray.direction = math.normalize(vec3(ray_end_ws - ray_start_ws))
    ray.direction = ray.direction * self.sector_camera_.clip.y
    return ray
end

function client:process_view(dt)

    local mouse_x = sf.Mouse.get_position(self).x
    local mouse_y = sf.Mouse.get_position(self).y
    
    if self:has_focus() then

        if sf.Mouse.is_button_pressed(sf.Mouse.Left) then
        
            if self.picking_constraint_ ~= nil then       
                local ray = self:screen_to_world_ray(mouse_x, self.h - mouse_y)
                local dir = math.normalize(ray.direction - ray.origin) * self.old_picking_dist_           
                self.picking_constraint_:set_pivot_b(ray.origin + dir)
            end
        end
    
        if sf.Mouse.is_button_pressed(sf.Mouse.Right) then
        
            local delta = vec2()
            delta.x = mouse_x - self.last_mouse_pos_.x
            delta.y = mouse_y - self.last_mouse_pos_.y
            self.sector_camera_:rotate(delta)
        end
        
        local speed = (sf.Keyboard.is_key_pressed(sf.Keyboard.LShift) and 1.5 or 0.1)
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
    
	self.sector_camera_.position = self.player_.position + vec3(0.0, 0.9, 0.0)
    self.sector_camera_:refresh()
    
	self.planet_camera_:copy(self.sector_camera_)
    self.planet_camera_.position = self.sector_camera_.position + self.scene_.planet_.loaded_sectors_[1].deformed_position_
	self.planet_camera_:refresh()
    
    self.last_mouse_pos_ = vec2(mouse_x, mouse_y)
end

function client:process_event(event)

    if event.type == sf.Event.MouseButtonPressed and event.mouse_button.button == sf.Mouse.Left then
    
        local ray = self:screen_to_world_ray(event.mouse_button.x, self.h - event.mouse_button.y)
        local ray_cb = self.dynamic_world_:ray_test(ray.origin, ray.direction)
        
        if ray_cb:has_hit() and ray_cb.body and ray_cb.body:is_static_object() == false then   
        
            self.picked_body_ = ray_cb.body
            local local_pivot = vec3(math.inverse(self.picked_body_.center_of_mass) * vec4(ray_cb.hit_point, 1))         
            self.picked_body_:set_activation_state(bt.RigidBody.DISABLE_DEACTIVATION)
            
            self.picking_constraint_ = bt.Point2PointConstraint(self.dynamic_world_, self.picked_body_, local_pivot)
            
            self.old_picking_pos_ = ray.direction
            self.hit_pos_ = ray_cb.hit_point
            self.old_picking_dist_ = math.distance(ray_cb.hit_point, ray.origin)
        end
    end
    
    if event.type == sf.Event.MouseButtonReleased and event.mouse_button.button == sf.Mouse.Left then
    
        if self.picking_constraint_ ~= nil then
        
            self.picking_constraint_:remove()
            self.picking_constraint_ = nil
            
            self.picked_body_:activate()
            self.picked_body_ = nil
        end
    end
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
        
        self:process_event(self.event_)
    end
    
    self:set_active(true)
    self:process_view(dt)
    
    gl.Viewport(0, 0, self.w, self.h)   
    local final_tex = self.render_pipeline_:render(self.planet_camera_, self.sector_camera_,
    function(planet_camera, sector_camera)
        self.scene_:draw(planet_camera, sector_camera)
    end)
    
    gl.ActiveTexture(gl.TEXTURE0)
    root.Shader.get():uniform("u_RenderingType", 2)
    root.Shader.get():sampler("s_Tex0", 0)
    final_tex:bind()
    
    self.render_pipeline_.screen_quad_:draw()
    self.canvas_:draw()
    self:display()
end

instance = client(1680, 1050, "Client", sf.Window.Fullscreen)