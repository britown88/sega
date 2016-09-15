--openEditor()
map.setTileSchema("default")

loadMap 'testWilderness'
--map.setAmbient(0)
--setPalette 'night'
player:teleport(31, 15)
--toggleStats();


--require 'lua.test'
--player:pushScript(testText)

--toggleLightMode()
--openEditor()
--map.setAmbient(0)

--time.jumpHours(12)
--time.pause()
--openEditor()
--toggleLightMode()

function reloadSchemas()
  reload 'img'
  reload 'img.sprites'

  img.clearImageCache()
  img.clearSpriteCache()

  reload 'maps.schemas'
  map.setTileSchema 'default'
end
