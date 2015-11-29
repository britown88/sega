Actions = {}

function Actions.move(actor, x, y)
  actor:move(x, y)
  while(actor:isMoving()) do
    coroutine.yield()
  end
end

function Actions.moveRelative(actor,x, y)
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

function Actions.scatter()
  for a=1, #Actors do
    local actor = Actors[a]
    if(actor ~= Player) then
      actor:pushScript(Actions.move, rand(0,21), rand(0,11))
    end
  end
end
