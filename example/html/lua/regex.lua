mongols_res:header('Content-Type','text/plain;charset=UTF-8')

if mongols_req:has_form('test') then
    local test=mongols_req:get_form('test')
    if mongols_regex.partial_match('^[0-9]+$',test) then
        mongols_res:content('int type: '..test)
    elseif mongols_regex.partial_match('^[a-zA-Z]+$',test) then
        mongols_res:content('string type: '..test)
    else
        local match=mongols_regex.match('(\\w+)[_:+-](\\w+)',test)
        if #match > 0 then
            local content=''
            for key,value in ipairs(match) do 
                content =content .. 'part '..key ..' = '.. value..'\n'
            end
            mongols_res:content('match: \n'..content)
        else
            mongols_res:content('not match: '.. test)
        end
    end
else 
    mongols_res:content('not found test variable')
end


mongols_res:status(200)

