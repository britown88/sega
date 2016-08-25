--require 'maps.schemas'

map = map or {}
map.directory = "assets/maps/"

function loadMap(fname)
  local path = string.format("%s%s.map", map.directory, fname)
  map.load(path)
  console.print(string.format("Loaded map: [c=0,5]%s[/c]", path))
end

function saveMap(fname)
  local path = string.format("%s%s.map", map.directory, fname)
  map.save(path)
  console.print(string.format("Saved map: [c=0,5]%s[/c]", path))
end
