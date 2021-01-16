local template = require "resty.template"

local view=template.new([[
<!DOCTYPE html>
<html>
<body>
  <p>{{message}}</p>
</body>
</html>]])

view.message='hello,world'

mongols_res:content(tostring(view))
mongols_res:status(200)