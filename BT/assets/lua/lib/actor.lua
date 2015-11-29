Actor = {}

function Actor:pushScript(script, ...)
  local co = coroutine.create(script)
  status, err = coroutine.resume(co, self, ...)
  if(not status) then
    error(err)
  end
  self.scripts[#self.scripts+1] = co
end

function Actor:popScript(script, ...)
  if(#self.scripts > 0) then
      self.scripts[#self.scripts] = nil
  end
end

function Actor:stepScript()
  local index = #self.scripts;
  if(index > 0) then
    local co = self.scripts[index];
    status, err = coroutine.resume(co)
    if(not status) then
      table.remove(self.scripts, index)
      error(err)
    end
    if(coroutine.status(co) == "dead") then
      table.remove(self.scripts, index)
    end
  end
end
