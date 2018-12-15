var route = function () {
    this.instance = null
    this.map = {}
    this.add = function (method, pattern, callback) {
        if (typeof (this.map[pattern]) == "undefined") {
            var ele = {}
            ele.method = method
            ele.pattern = pattern
            ele.callback = callback
            this.map[pattern] = ele
        }
    }
    this.run = function (req, res, param) {
        var x = null
        for (x in this.map) {
            var reg = new RegExp(x, 'ig');
            var param = req.uri().match(reg)
            if (param != null) {
                var ele = this.map[x]
                if (ele.method.indexOf(req.method()) >= 0) {
                    ele.callback(req, res, param)
                    break
                }
            }
        }
    }
}

route.get_instance = function () {
    if (this.instance == null) {
        this.instance = new route()
    }
    return this.instance;
}

module.exports = route
