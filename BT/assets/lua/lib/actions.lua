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
