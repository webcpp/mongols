local echo=require('echo')
medis.CMD('SET',medis.ARGS[3],echo.concat('hello,',medis.ARGS[4]))
medis.RESULT={a='hello',b='world',1,3.5,'test'}
