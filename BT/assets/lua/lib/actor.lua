Actor = {}

Actor.new = New
Actor.scripts = {}

function Actor:pushScript(script, ...)
  local co = coroutine.create(script)
  coroutine.resume(co, self, ...)
  self.scripts[#self.scripts+1] = co
end

function Actor:stepScript()
  local index = #self.scripts;
  if(index > 0) then
    local co = self.scripts[index];
    coroutine.resume(co)
    if(coroutine.status(co) == "dead") then
      table.remove(self.scripts, index)
    end
  end
end
