local route={map={}}

function route:new()
    local cls={}
    setmetatable(cls, self)
    self.__index = self
    return cls
end

function route:get_instance()
    if self.instance == nil then
        self.instance=self:new()
    end
    return self.instance
end

function route:free()
    if self.instance then
        self.map=nil
        self.instance=nil
    end
end


function route:add(method,pattern,callback)
    local ele={}
    ele.method=method
    ele.pattern=pattern
    ele.callback=callback
    if self.map[pattern]==nil then
        self.map[pattern]=ele
    end
end


function route:run(req,res,param)
    for i,ele in pairs(self.map)
    do
        for j,m in pairs(ele.method)
        do 
            if m==req:method() then
                param=mongols_regex.match(ele.pattern,req:uri())
                if #param > 0 then
                    ele.callback(req,res,param)
                    return 
                end
            end
        end
    end 
end

return route