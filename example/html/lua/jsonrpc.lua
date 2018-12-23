-- curl -i -X POST -H 'Content-Type: application/json' --data '{"method":"post"}'  http://127.0.0.1:9090/jsonrpc.lua
local cjson=require('cjson')

if mongols_req:has_form('__body__') 
then
    local body = cjson.decode(mongols_req:get_form('__body__'))
    local result={}
    result['jsonrpc']=2.0
    result['method']=body['method']
    mongols_res:content(cjson.encode(result))
    mongols_res:status(200)
    mongols_res:header('Content-Type','application/json')
else
    mongols_res:status(500)
    mongols_res:content('bad request.')
end
    
