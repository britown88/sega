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
      if(type(err) ~= "string") then
        Console.print("[c=0,13][=]Unspecified Error stepping coroutine[/=][/c]")
      else
        Console.print(string.format("[c=0,13][=]Error stepping coroutine:\n%q[/=][/c]", err))
      end
    end

  end
end
