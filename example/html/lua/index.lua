--
local echo=require('echo')
mongols_res:header('Content-Type','text/plain;charset=UTF-8')
mongols_res:content(echo.concat('hello,','world'))
mongols_res:status(200)
--


--[[

local template = require "resty.template"
local view=template.new('name: {{name}}\
age: {{age}}\
score: {{score}}\
text:{{text}}\
text_md5: {{md5}}\
text_sha1: {{sha1}}')

local text='hello,world'

local s=studest.new()
s:set_score(74.6)
s:set_name("Jerry")
s:set_age(14)

view.name=s:get_name()
view.age=s:get_age()
view.score=s:get_score()
view.text=text
view.md5=md5(text)
view.sha1=sha1(text)

mongols_res:header('Content-Type','text/plain;charset=UTF-8')
mongols_res:content(tostring(view))
mongols_res:status(200)

--]]

--[[

local echo=require('echo')
local route=require('route'):get_instance()


route:add({'GET'},'^/hello/([0-9a-zA-Z]+)/?$'
,function(req,res,param)
    res:content('hello,'..param[2])
    res:header('Content-Type','text/plain;charset=UTF-8')
    res:status(200)
end)

route:add({'GET'},'^/([0-9]+)/?$'
,function(req,res,param)
    res:content(param[2]) 
    res:header('Content-Type','text/plain;charset=UTF-8')
    res:status(200)
end)


route:add({'GET','POST','PUT'},'^/(\\w+)/?$'
,function(req,res,param) 
    local text= echo.concat('uri: ',req:uri(),'\n','method: ',req:method(),'\n',param[2])
    res:content(text) 
    res:header('Content-Type','text/plain;charset=UTF-8')
    res:status(200)
end)

route:run(mongols_req,mongols_res)

--]]
