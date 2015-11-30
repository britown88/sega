shirley = {}

function shirley.toLook(actor)
  actor:stop()

  textAreas.smallbox:push("How dare you!")

  local time, delay = actor:moveSpeed()
  actor:setMoveSpeed(100, 0)

  local px, py = player:position()
  actions.move(actor, px, py)
  local x,y = actor:position()

  local dx, dy = 0, 0

  if(x > px) then dx = -1 end
  if(x < px) then dx = 1 end
  if(y > py) then dy = -1 end
  if(y < py) then dy = 1 end

  if(rand(0,2) == 0) then
    textAreas.smallbox:push("[c=15,4]SMAAAASH!![/c]")
    local ptime, pdelay = player:moveSpeed()
    player:setMoveSpeed(0, 0)
    actions.moveRelative(player, dx, dy)
    player:setMoveSpeed(ptime, pdelay)
  else
    textAreas.smallbox:push("*slap*")
  end

  actor:setMoveSpeed(time, delay)
end


function testShirley(actor)
  actor:setMoveSpeed(500,1000)
  actor:pushScript(actions.wander, 3, 3)
  actor.response = shirley
end
