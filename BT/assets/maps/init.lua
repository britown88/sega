require 'maps.schemas'

maps = maps or {}
maps.directory = "assets/maps/"

function loadMap(fname)
  local path = string.format("%s%s.map", maps.directory, fname)
  maps.load(path)
  console.print(string.format("Loaded map: [c=0,5]%s[/c]", path))
end

function saveMap(fname)
  local path = string.format("%s%s.map", maps.directory, fname)
  maps.save(path)
  console.print(string.format("Saved map: [c=0,5]%s[/c]", path))
end
