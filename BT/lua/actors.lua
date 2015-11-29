actors = {}
function actors:add(a)
  for i=1, #self do
    if(self[i].entity == a.entity) then return end
  end
  self[#self + 1] = a
end

function actors:remove(a)
  for i=1, #self do
    if(self[i].entity == a.entity) then
      table.remove(self, i)
      return
    end
  end
end

function actors:get(a)
  for i=1, #self do
    if(self[i].entity == a.entity) then
      return self[i]
    end
  end
  return nil
end
