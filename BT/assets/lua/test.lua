test = {}

function test.toLook(actor)
  actions.stop(actor)
  local i, result = promptChoices{"At their face.", "At their butt.", "At their feet."}
  textAreas.smallbox:push("Hey stop looking " .. result)
end

function test.toTalk(actor)
  actions.stop(actor)
  textAreas.smallbox:push("I don't want to talk to you!")
end

function test.toUse(actor)
  actions.stop(actor)
  textAreas.smallbox:push("I will not be used!")
end

function test.toFight(actor)
  actions.stop(actor)
  textAreas.smallbox:push("Come at me bro!")
  local px,py = player:position()
  actions.move(actor, px, py)
end
