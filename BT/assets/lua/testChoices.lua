function textAndWait(txt)
  textAreas.smallbox:push(txt)
  textAreas.smallbox:wait()
end

function testChoices(actor)
  -- first lets get the players position
  local px, py = player:position()

  --and move there
  actions.move(actor, px, py)

  --greet us!
  textAndWait "What can I do for you, stranger?"

  local choices = {
    "Ask about town",
    "Ask about the wilderness",
    "Ask about adventure!"
  }
  --let us choose what to ask them and respond accordingly
  local i, r = promptChoices(choices) --the index and full text

  if i == 0 then
    textAndWait "Oh the town lies to the northwest."
  elseif i == 1 then
    textAndWait "It's dangerous out there!"
  elseif i == 2 then
    textAndWait "You like to dance close to the fire don't you!"
  end

  actions.wait(2.0)
  --wish us the best of luck
  textAndWait "Well, good luck to  you!"
end

function choiceTest(actor)
  actor.response = {toTalk = testChoices}
end
