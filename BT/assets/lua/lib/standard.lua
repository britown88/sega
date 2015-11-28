function New(self, o)
 o = o or {}   -- create object if user does not provide one
 setmetatable(o, self)
 self.__index = self
 return o
end

function cls()
  Console.clear()
end

function dir(o)
  for k,v in pairs(o) do
    Console.print(string.format("[%q: %q]", k, type(v)))
  end
end

function reload(m)
  if(not package.loaded[m]) then
    error("Module is not loaded!")
  end

  package.loaded[m] = nil
  require(m)
  Console.print("Module reloaded!")
end
