--
local echo=require('echo')
mongols_res:header('Content-Type','text/plain;charset=UTF-8')
mongols_res:content(echo.concat('hello,','world'))
mongols_res:status(200)
--

--[[

local lfs = require"lfs"
local echo=require('echo')

local content=''
function attrdir (path)
    for file in lfs.dir(path) do
        if file ~= "." and file ~= ".." then
            local f = path..'/'..file
            content=echo.concat(content,f,'\n')
            local attr = lfs.attributes (f)
            assert (type(attr) == "table")
            if attr.mode == "directory" then
                attrdir (f)
            else
                for name, value in pairs(attr) do
                    content=echo.concat(content,name,'\t',value,'\n')
                end
            end
        end
    end
end

attrdir (".")

mongols_res:header('Content-Type','text/plain;charset=UTF-8')
mongols_res:content(content)
mongols_res:status(200)

--]]

--[[
local lustache = require('lustache')
local view_model = {
  title = "Joe",
  calc = function ()
    return 2 + 4
  end
}
local output = lustache:render("{{title}} spends {{calc}}", view_model)
mongols_res:header('Content-Type','text/plain;charset=UTF-8')
mongols_res:content(output)
mongols_res:status(200)

--]]

--[[
local m = require('hello')
mongols_res:header('Content-Type','text/plain;charset=UTF-8')
mongols_res:content(m.hello())
mongols_res:status(200)
--]]


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



--[[

mongols_res:header('Content-Type','text/plain;charset=UTF-8')
mongols_res:status(200)
local host="www.baidu.com"
local port=443
local enable_ssl =true
local cli=mongols_tcp_client.new(host,port,enable_ssl)
local req="GET / HTTP/1.1\r\n\r\n"

if cli:ok() then
    if cli:send(req) then
        mongols_res:content(cli:recv(8096))
    else
        mongols_res:content("send failed.")
    end
else
    mongols_res:content("connection failed.")
end

--]]
