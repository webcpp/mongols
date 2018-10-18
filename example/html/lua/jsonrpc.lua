-- curl -i -X POST -H 'Content-Type: application/json' --data '{"method":"post"}'  http://127.0.0.1:9090/jsonrpc.lua
local json=require('JSON')

if mongols_req:has_form('__body__') 
then
    local body = json:decode(mongols_req:get_form('__body__'))
    local result={}
    result['jsonrpc']=2.0
    result['method']=body['method']
    mongols_res:content(json:encode(result))
    mongols_res:status(200)
    mongols_res:header('Content-Type','application/json')
else
    mongols_res:status(500)
    mongols_res:content('bad request.')
end
    
