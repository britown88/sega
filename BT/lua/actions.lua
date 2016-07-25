actions = {}

function actions.waitForStop(actor)
  while(actor:isMoving()) do
    coroutine.yield()
  end
end

function actions.persistentMove(actor, x, y, range)
  range = range or 0
  repeat
    actions.move(actor, x, y)
    coroutine.yield()
  until(actor:distanceTo(x, y) <= range)
end

function actions.move(actor, x, y)
  actor:move(x, y)
  actions.waitForStop(actor)
end

function actions.wait(actor, t)
  local startTime = os.clock()
  while(os.clock() - startTime < t) do
    coroutine.yield()
  end
end

function actions.moveRelative(actor, x, y)
    actor:moveRelative(x, y)
    actions.waitForStop(actor)
end

function actions.stop(actor)
  actor:stop()
  actions.waitForStop(actor)
end

function actions.wander(actor, width, height)
  local x,y = actor:position()
  local left = math.max(0, x - math.floor(width/2))
  local top = math.max(0, y - math.floor(height/2))
  local right = left + width
  local bottom = top + height

  while(true) do
    local posx, posy = rand(left, right), rand(top, bottom)
    actions.move(actor, posx, posy, x - left)
    coroutine.yield()
  end
end

function actions.scatter()
  for a=1, #actors do
    local actor = actors[a]
    if(actor ~= player) then
      actor:setMoveSpeed(500,0)
      actor:pushScript(actions.wander, 5, 5)
    end
  end
end
