local echo=require('echo')
medis.CMD('SET',medis.ARGS[3],echo.concat('hello,',medis.ARGS[4]))
