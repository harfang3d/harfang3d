hg = require("harfang")

local bin = {}
local path_sep = package.config:sub(1, 1)
local here = hg.CutFileName(debug.getinfo(1,'S').source)
if here:find('@', 1, true) == 1 then
	here = here:sub(2,-1)
end

-- [todo] check against a list (like python) ?
setmetatable(bin, 
	{
		__index = function(t, name)
			t[name] = function(...)
				local path = table.concat({here, name, name}, path_sep)
				if not hg.Exists(path) then
					path = path .. '.exe'
					if not hg.Exists(path) then
						path = nil
					end
				end
				assert(path, "Unknown tool " .. name)
				return os.execute(table.concat({path, ...}, ' '))
			end
			return rawget(t, name)
		end
	}
)

return bin
