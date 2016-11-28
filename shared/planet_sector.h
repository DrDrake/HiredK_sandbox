--[[
- @file planet_sector.h
- @brief
]]

class 'planet_sector' (root.ScriptObject)

function planet_sector:__init(planet, terrain, position, deformed_position, index, path)
    root.ScriptObject.__init(self, "planet_sector")
    
    self.planet_ = planet
    self.terrain_ = terrain
    self.position_ = position
    self.deformed_position_ = deformed_position
    self.index_ = index
    self.path_ = path
    self.paged_in_ = false
    
    self.global_cubemap_ = cubemap_object(planet.scene_, 128)
    self.objects_ = {}
end

function planet_sector:parse()

    local sector_config = json.parse(FileSystem:search(self.path_, true))  
    local objects_array = sector_config:get_array("objects")
    for i = 1, objects_array.size do
    
        local object_config = objects_array:get(i - 1)
        local type = object_config:get_number("type")
        local object = nil
        
        if type == 1 then
            object = cube_object(self.planet_.scene_)
        end 
        
        if type == 2 then
            local size = object_config:get_number("size")
            object = cubemap_object(self.planet_.scene_, size)
        end
        
        if type == 3 then
            local path = object_config:get_string("path")
            object = model_object(self.planet_.scene_, path)
        end
        
        object:parse(object_config)
        table.insert(self.objects_, object)
    end
end

function planet_sector:write()

    local sector_config = json.Object() 
    
    local objects_array = json.Array()
    for i = 1, table.getn(self.objects_) do
    
        local object_config = json.Object()
        self.objects_[i]:write(object_config)
        objects_array:push(object_config)
    end
    
    sector_config:set_array("objects", objects_array)
    local data = json.Value(sector_config):serialize(true)
    FileSystem:write(self.path_, data)
end

function planet_sector:page_in()

    if self.paged_in_ ~= true then
        print("PAGE_IN", self.index_, self.position_.x, self.position_.y, self.position_.z)
        self.paged_in_ = true
    end
end

function planet_sector:draw(camera)

    if self.paged_in_ then
        for i = 1, table.getn(self.objects_) do
            self.objects_[i]:draw(camera)
        end
    end
end

function planet_sector:draw_shadow(matrix)

    if self.paged_in_ then
        for i = 1, table.getn(self.objects_) do
            self.objects_[i]:draw_shadow(matrix)
        end
    end
end