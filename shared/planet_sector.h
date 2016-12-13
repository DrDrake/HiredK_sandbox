--[[
- @file planet_sector.h
- @brief
]]

class 'planet_sector' (object_base)

function planet_sector:__init(planet, face, deformed)
    object_base.__init(self, planet)
    
    self.planet_ = planet
    self.face_ = face
    self.deformed_ = deformed
    self.objects_ = {}
end

function planet_sector:page_in()
end

function planet_sector:__draw(camera)

    for i = 1, table.getn(self.objects_) do
        self.objects_[i]:__draw(camera)
    end
end

function planet_sector:__draw_shadow(matrix)

    for i = 1, table.getn(self.objects_) do
        self.objects_[i]:__draw_shadow(matrix)
    end
end