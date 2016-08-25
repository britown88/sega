require 'img'
require 'pal'

db.beginTransaction()

db.insertTileSchema {
  set = "default", tiles = {
    {sprite = 'tiles.blank'},
    {sprite = 'tiles.grass1'},
    {sprite = 'tiles.grass2'},
    {sprite = 'tiles.grass3'},
    {sprite = 'tiles.tree1'},
    {sprite = 'tiles.tree2'},
    {sprite = 'tiles.water1'},
    {sprite = 'tiles.mountain1', occlusion = true},
    {sprite = 'tiles.wall', occlusion = true},
    {sprite = 'tiles.torch', lit = true, radius = 5, fadeWidth = 3, centerLevel = 6},
    {sprite = 'tiles.bridge'},
    {sprite = 'tiles.water2'},
    {sprite = 'tiles.wcorner1'},
    {sprite = 'tiles.campfire', lit = true, radius = 5, fadeWidth = 3, centerLevel = 6},
    {sprite = 'tiles.mountain2', occlusion = true},
    {sprite = 'tiles.wcorner2'},
    {sprite = 'tiles.wcorner3'},
    {sprite = 'tiles.wcorner4'}
  }
}

db.endTransaction()
