--[[
local echo=require('echo')
mongols_res:header('Content-Type','text/plain;charset=UTF-8')
mongols_res:content(echo.concat('hello,','world'))
mongols_res:status(200)
--]]

local route=require('route')

route.add({'GET'},'^/([0-9]+)/?$'
,function(req,res,param)
    res:content(param[2]) 
    res:header('Content-Type','text/plain;charset=UTF-8')
    res:status(200)
end)


route.add({'GET','POST','PUT'},'^/(.*)$'
,function(req,res,param) 
    local text='uri: '..req:uri()..'\n'..'method: '..req:method()..'\n'..param[2]
    res:content(text) 
    res:header('Content-Type','text/plain;charset=UTF-8')
    res:status(200)
end)

route.run(mongols_req,mongols_res,{})


