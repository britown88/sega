actors = {}
function actors:add(a)
  for i=1, #self do
    if(self[i].actor == a.actor) then return end
  end
  self[#self + 1] = a
end

function actors:remove(a)
  for i=1, #self do
    if(self[i].actor == a.actor) then
      table.remove(self, i)
      return
    end
  end
end

function actors:get(a)
  for i=1, #self do
    if(self[i].actor == a.actor) then
      return self[i]
    end
  end
  return nil
end

function actors:indexOf(a)
  for i=1, #self do
    if(self[i].actor == a.actor) then
      return i
    end
  end
  return 0
end
