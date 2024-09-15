--[[
Author       : muqiu0614 3155833132@qq.com
Date         : 2024-09-15 17:19:45
LastEditors  : muqiu0614 3155833132@qq.com
LastEditTime : 2024-09-15 17:22:06
FilePath     : /myworkflow/test/test.lua
Description  :
Copyright (c) 2024 by muqiu0614 email: 3155833132@qq.com, All Rights Reserved.
--]]


abc = require("abc")
function max(a, b)
    if a > b then
        print(a)
    else
        print(b)
    end
end

Aa = 10
Ab = 20

abc.gettime()
max(Aa, Ab)


