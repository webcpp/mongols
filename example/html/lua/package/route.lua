local route={}
route.map={}

function route.add(method,pattern,callback)
    local ele={}
    ele.method=method
    ele.pattern=pattern
    ele.callback=callback
    if route.map[pattern]==nil then
        route.map[pattern]=ele
    end
end


function route.run(req,res,param)
    for i,ele in pairs(route.map)
    do
        for j,m in pairs(ele.method)
        do 
            if m==req:method() then
                param=mongols_regex.match(ele.pattern,req:uri())
                if #param > 0
                then
                    ele.callback(req,res,param)
                    break
                end
                break
            end
        end
    end 
end

return route