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

function Actor:coMove(x, y)
  self:move(x, y)
  while(self:isMoving()) do
    coroutine.yield()
  end
end

function Actor:coStop()
  self:stop()
  while(self:isMoving()) do
    coroutine.yield()
  end
end

Actors = {}
function Actors:add(a)
  for i=1, #self do
    if(self[i].entity == a.entity) then return end
  end
  self[#self + 1] = a
end

function Actors:remove(a)
  for i=1, #self do
    if(self[i].entity == a.entity) then
      table.remove(self, i)
      return
    end
  end
end

function Actors:get(a)
  for i=1, #self do
    if(self[i].entity == a.entity) then
      return self[i]
    end
  end
  return nil
end

function Actors:stepScripts()
  for i=1, #self do

    status, err = pcall(self[i].stepScript, self[i])
    if(not status) then
      Console.print(string.format("[c=0,13][=]Error stepping coroutine:\n%q[/=][/c]", err))
    end

  end
end
