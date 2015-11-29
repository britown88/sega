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
