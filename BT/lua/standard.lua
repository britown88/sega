function new(self, o)
 o = o or {}   -- create object if user does not provide one
 setmetatable(o, self)
 self.__index = self
 return o
end

function cls()
  console.clear()
end

function dir(o)
  for k,v in pairs(o) do
    console.print(string.format("[%q: %q]", k, type(v)))
  end
end

function reload(m)
  package.loaded[m] = nil
  require(m)
  console.print("Module reloaded!")
end
