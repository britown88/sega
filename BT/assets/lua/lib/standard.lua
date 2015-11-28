function New(self, o)
 o = o or {}   -- create object if user does not provide one
 setmetatable(o, self)
 self.__index = self
 return o
end

function cls()
  Console.clear()
end
