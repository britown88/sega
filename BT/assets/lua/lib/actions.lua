Actions = {}

function Actions.move(actor, x, y, range)
  range = range or 0
  repeat
    actor:move(x, y)
    while(actor:isMoving()) do
      coroutine.yield()
    end
  until(actor:distanceTo(x, y) <= range)
end

function Actions.moveRelative(actor, x, y)
    actor:moveRelative(x, y)
    while(actor:isMoving()) do
      coroutine.yield()
    end
end

function Actions.stop(actor)
  actor:stop()
  while(actor:isMoving()) do
    coroutine.yield()
  end
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
    coroutine.yield()
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
