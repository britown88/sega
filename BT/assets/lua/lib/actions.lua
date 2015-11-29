Actions = {}

function Actions.waitForStop(actor)
  while(actor:isMoving()) do
    coroutine.yield()
  end
end

function Actions.persistentMove(actor, x, y, range)
  range = range or 0
  repeat
    Actions.move(actor, x, y)
  until(actor:distanceTo(x, y) <= range)
end

function Actions.move(actor, x, y)
  actor:move(x, y)
  Actions.waitForStop(actor)
end

function Actions.moveRelative(actor, x, y)
    actor:moveRelative(x, y)
    Actions.waitForStop(actor)
end

function Actions.stop(actor)
  actor:stop()
  Actions.waitForStop(actor)
end

function Actions.wander(actor, width, height)
  local x,y = actor:position()
  local left = math.max(0, x - math.floor(width/2))
  local top = math.max(0, y - math.floor(height/2))
  local right = left + width
  local bottom = top + height

  while(true) do
    local posx, posy = rand(left, right), rand(top, bottom)
    Actions.move(actor, posx, posy, x - left)
  end
end

function Actions.scatter()
  for a=1, #Actors do
    local actor = Actors[a]
    if(actor ~= Player) then
      actor:pushScript(Actions.move, rand(0,21), rand(0,11))
    end
  end
end

function test(index)
  Actors[index]:pushScript(Actions.move, rand(0,21), rand(0,11))
end
