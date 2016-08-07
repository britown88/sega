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

function testText(actor)
  actions.move(player, 10, 10);
  textAreas.smallbox:push("Let's wait a second. THis message should scroll a little bit so we have to talk a bit oh boy isnt it a beautiful day today i think so yess ok.")
  textAreas.smallbox:wait()
  actions.wait(1)
  textAreas.smallbox:push("Ok let's go")
  textAreas.smallbox:wait()
  actions.move(player, 1, 1);
  textAreas.smallbox:push("And done!")
end
