db.beginTransaction()

db.insertSprite{
   id = 'splash.fire', size = {167, 28},
   frames = {
      {image = 'splash.fire',
       pos = {0, 0}}, {pos = {0, 1}}, {pos = {0, 2}}, {pos = {0, 3}}, {pos = {0, 4}},
      {pos = {0, 5}}, {pos = {0, 6}}, {pos = {0, 7}}, {pos = {0, 8}}, {pos = {0, 9}}
   }
}

--TILES FOR DAYS
db.insertSprite{ id = 'tiles.blank',  size = {14, 14}, frames = {{image = 'tiles', pos = {0, 0}} }}
db.insertSprite{ id = 'tiles.grass1',  size = {14, 14}, frames = {{image = 'tiles', pos = {2, 1}} }}
db.insertSprite{ id = 'tiles.grass2',  size = {14, 14}, frames = {{image = 'tiles', pos = {2, 0}} }}
db.insertSprite{ id = 'tiles.grass3',  size = {14, 14}, frames = {{image = 'tiles', pos = {1, 0}} }}
db.insertSprite{ id = 'tiles.tree1',  size = {14, 14}, frames = {{image = 'tiles', pos = {3, 0}} }}
db.insertSprite{ id = 'tiles.tree2',  size = {14, 14}, frames = {{image = 'tiles', pos = {4, 0}} }}
db.insertSprite{ id = 'tiles.water1',  size = {14, 14}, frames = {{image = 'tiles', pos = {5, 0}}, {pos = {5, 1}} }}
db.insertSprite{ id = 'tiles.water2',  size = {14, 14}, frames = {{image = 'tiles', pos = {10, 0}}, {pos = {10, 1}} }}
db.insertSprite{ id = 'tiles.mountain1',  size = {14, 14}, frames = {{image = 'tiles', pos = {6, 0}} }}
db.insertSprite{ id = 'tiles.mountain2',  size = {14, 14}, frames = {{image = 'tiles', pos = {6, 1}} }}
db.insertSprite{ id = 'tiles.wcorner1',  size = {14, 14}, frames = {{image = 'tiles', pos = {11, 0}}, {pos = {11, 1}} }}
db.insertSprite{ id = 'tiles.wcorner2',  size = {14, 14}, frames = {{image = 'tiles', pos = {12, 0}}, {pos = {12, 1}} }}
db.insertSprite{ id = 'tiles.wcorner3',  size = {14, 14}, frames = {{image = 'tiles', pos = {13, 0}}, {pos = {13, 1}} }}
db.insertSprite{ id = 'tiles.wcorner4',  size = {14, 14}, frames = {{image = 'tiles', pos = {14, 0}}, {pos = {14, 1}} }}
db.insertSprite{ id = 'tiles.campfire',  size = {14, 14}, frames = {{image = 'tiles', pos = {0, 1}}, {pos = {0, 2}} }}
db.insertSprite{ id = 'tiles.bridge',  size = {14, 14}, frames = {{image = 'tiles', pos = {9, 0}} }}
db.insertSprite{ id = 'tiles.torch',  size = {14, 14}, frames = {{image = 'tiles', pos = {8, 0}}, {pos = {8, 1}} }}
db.insertSprite{ id = 'tiles.wall',  size = {14, 14}, frames = {{image = 'tiles', pos = {7, 0}} }}

db.endTransaction()
