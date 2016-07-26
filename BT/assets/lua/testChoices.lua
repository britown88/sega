function testChoices(actor)
  -- first lets get the players position
  local px, py = player:position()

  --and move there
  actions.move(actor, px, py)

  --greet us!
  textAreas.smallbox:push "What can I do for you, stranger?"

  actions.wait(2.0)--seconds

  local choices = {
    "Ask about town",
    "Ask about the wilderness",
    "Ask about adventure!"
  }

  --let us choose what to ask them and respond accordingly
  local i, r = promptChoices(choices) --the index and full text

  if i == 0 then
    textAreas.smallbox:push "Oh the town lies to the northwest."
  elseif i == 1 then
    textAreas.smallbox:push "It's dangerous out there!"
  elseif i == 2 then
    textAreas.smallbox:push "You like to dance close to the fire don't you!"
  end

  --wish us the best of luck
  actions.wait(2.0)--seconds
  textAreas.smallbox:push "Well, good luck to  you!"
  actions.wait(2.0)--seconds
end
