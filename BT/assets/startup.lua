--openEditor()
map.setTileSchema("default")

loadMap 'testWilderness'
map.setAmbient(0)
setPalette 'night'
player:teleport(6, 5)
--toggleStats();


--require 'lua.test'
--player:pushScript(testText)

--toggleLightMode()
--openEditor()
--map.setAmbient(0)

function reloadSchemas()
  reload 'maps.schemas'
  map.setTileSchema 'default'
end
