--[[
- @file planet_sector.h
- @brief
]]

class 'planet_sector' (root.ScriptObject)

function planet_sector:__init(planet, terrain, position, index)
    root.ScriptObject.__init(self, "planet_sector")
    self.planet_ = planet
    self.terrain_ = terrain
    self.position_ = position
    self.index_ = index
    self.paged_in_ = false
end

function planet_sector:page_in()

    if self.paged_in_ ~= true then
        self.gbuffer_shader_ = FileSystem:search("shaders/gbuffer.glsl", true)
        self.model_ = FileSystem:search("scene.glb", true)
        self.paged_in_ = true
        
        print("PAGE_IN", self.index_, self.position_.x, self.position_.y, self.position_.z)
    end
end

function planet_sector:draw(planet_camera, sector_camera)

    if self.paged_in_ then
    
        self.gbuffer_shader_:bind()       
        root.Shader.get():uniform("u_EnableLogZ", self.planet_.enable_logz_ and 1 or 0)       
        self.planet_.scene_:bind_global(planet_camera)
    
        self.model_:draw(sector_camera)     
        root.Shader.pop_stack()
    end
end