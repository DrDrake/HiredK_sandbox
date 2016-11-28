--[[
- @file editor.lua
- @brief
]]

#define WINDOW_TITLE "Planet Editor"

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

function editor:create_save_file_dialog(title, exts, default_name, cb)

	if self.save_file_dialog_ == nil or self.save_file_dialog_.hidden then
		self.save_file_dialog_ = save_file_dialog(self, self.canvas_, title, exts, default_name, function(path)	
		
			cb(path)
            
			self.save_file_dialog_:close_button_pressed()
			self.save_file_dialog_ = nil
		end)
	end
end

function editor:create_open_file_dialog(title, exts, cb)

	if self.open_file_dialog_ == nil or self.open_file_dialog_.hidden then
		self.open_file_dialog_ = open_file_dialog(self, self.canvas_, title, exts, function(path)	
		
			cb(path)
            
			self.open_file_dialog_:close_button_pressed()
			self.open_file_dialog_ = nil
		end)
	end
end

function editor:create_new_planet()

	if self.new_planet_dialog_ == nil or self.new_planet_dialog_.hidden then
		self.new_planet_dialog_ = new_planet_dialog(self, self.canvas_, function(name)
        
            local default_name = string.gsub(string.lower(name), " ", "_") .. ".json"
            self:create_save_file_dialog("Save Planet As..", { "json" }, default_name, function(path)

                local planet = self.scene_:load_planet(path)
                planet:build(6360000.0, 14, 41223124)
                planet.name = name
                
                local sector = planet:load_sector(planet.faces_[3], vec3(0, 0, 0), "sandbox/sector.json")
                planet.sectors_point_cloud_:build()   
                
                local obj1 = model_object(self.scene_, "scene.glb")           
                table.insert(sector.objects_, obj1)
                
                local obj2 = model_object(self.scene_, "tree.glb")
                obj2.transform_.scale = vec3(3.0)
                table.insert(sector.objects_, obj2)
                
                local obj3 = model_object(self.scene_, "lucy.glb")
                obj3.transform_.scale = vec3(100.0)
                table.insert(sector.objects_, obj3)
                
                local obj4 = cube_object(self.scene_)
                obj4.transform_.position.y = 5.0
                table.insert(sector.objects_, obj4)
                
                local obj5 = cube_object(self.scene_)
                obj5.transform_.position.y = 8.0
                table.insert(sector.objects_, obj5)
                
                planet:write()
                
                self:refresh_gui()
            end)

			self.new_planet_dialog_:close_button_pressed()
			self.new_planet_dialog_ = nil
		end)
	end
end

function editor:open_planet()

	self:create_open_file_dialog("Open Planet", { "json" }, function(path)
    
        local planet = self.scene_:load_planet(path)   
        planet:parse()
        
        self:refresh_gui()
	end)
end

function editor:save_all()

    if self.scene_:is_planet_loaded() then
        self.scene_.planet_:write()
        
        self:refresh_gui()
    end
end

function editor:refresh_gui()

	if self.scene_:is_planet_loaded() then
        self:set_title(self.scene_.planet_.name .. " - " .. WINDOW_TITLE)
	else
		self:set_title(WINDOW_TITLE)
	end

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
    
    --self.scene_.sun_:show_depth_tex()
	
    self:display()
end

instance = editor(1000, 800, WINDOW_TITLE, sf.Window.Default)