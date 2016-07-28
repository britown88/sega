schemas = {
  {img = {0}}, --0, blank
  {img = {1}}, --1, grass 1
  {img = {18}}, --1, grass 2
  {img = {2}}, --2, grass 3
  {img = {3}}, --3, single tree
  {img = {4}}, --4, tree group
  {img = {5, 21}}, --5, water
  {img = {6}, occlusion = 1}, --6, mountain
  {img = {7}, occlusion = 1}, --7, wall
  {img = {8, 24},
    lit = true,
    radius = 5,
    fadeWidth = 3,
    centerLevel = 6},  --8, walltorch
  {img = {9}}, --7, bridge
  {img = {10, 26}}, --7, rapids
  {img = {11, 27}}, --7, corner
  {img = {16, 32},
    lit = true,
    radius = 5,
    fadeWidth = 3,
    centerLevel = 6} --7, campfire
}



map.setSchemas(schemas)
