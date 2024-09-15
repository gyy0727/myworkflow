--[[
Author       : muqiu0614 3155833132@qq.com
Date         : 2024-09-15 17:33:20
LastEditors  : muqiu0614 3155833132@qq.com
LastEditTime : 2024-09-15 17:38:03
FilePath     : /myworkflow/test/abc.lua
Description  :
Copyright (c) 2024 by muqiu0614 email: 3155833132@qq.com, All Rights Reserved.
--]]


abc = {
    gettime = function()
        time = os.time()
        print(time)
    end
}

return abc