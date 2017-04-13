local function addTile(name, posList)
  local sprite = { id = name,  size = {14, 14}, frames = {{image = 'tiles', pos = posList[1]} }}

  if #posList > 1 then
    for i = 2, #posList do
      sprite.frames[i] = {pos = posList[i]}
    end
  end

  db.insertSprite(sprite)
end

db.beginTransaction()

db.insertSprite{
   id = 'splash.fire', size = {167, 28},
   frames = {
      {image = 'splash.fire',
       pos = {0, 0}}, {pos = {0, 1}}, {pos = {0, 2}}, {pos = {0, 3}}, {pos = {0, 4}},
      {pos = {0, 5}}, {pos = {0, 6}}, {pos = {0, 7}}, {pos = {0, 8}}, {pos = {0, 9}}
   }
}

addTile('tiles.blank', {{0, 0}})
addTile('tiles.grass1', {{2, 1}})
addTile('tiles.grass2', {{2, 0}})
addTile('tiles.grass3', {{1, 0}})
addTile('tiles.tree1', {{3, 0}})
addTile('tiles.tree2', {{4, 0}})
addTile('tiles.water1', {{5, 0}, {5, 1}})
addTile('tiles.water2', {{10, 0}, {10, 1}})
addTile('tiles.mountain1', {{6, 0}})
addTile('tiles.mountain2', {{6, 1}})
addTile('tiles.wcorner1', {{11, 0}, {11, 1}})
addTile('tiles.wcorner2', {{12, 0}, {12, 1}})
addTile('tiles.wcorner3', {{13, 0}, {13, 1}})
addTile('tiles.wcorner4', {{14, 0}, {14, 1}})
addTile('tiles.campfire', {{0, 1}, {0, 2}})
addTile('tiles.bridge', {{9, 0}})
addTile('tiles.torch', {{8, 0}, {8, 1}})
addTile('tiles.wall', {{7, 0}})
addTile('tiles.path1', {{2, 2}})
addTile('tiles.path2', {{3, 2}})
addTile('tiles.path3', {{2, 3}})
addTile('tiles.path4', {{3, 3}})
addTile('tiles.path5', {{2, 4}})
addTile('tiles.path6', {{3, 4}})

db.endTransaction()
