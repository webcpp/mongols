local echo =  {}

function echo.concat(...)
    local text=''
    for i,v in ipairs({...}) do
        text=text..tostring(v)
    end
   return text
end

return echo  
