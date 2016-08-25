
db.beginTransaction()
db.insertImageFolder 'assets/img'
db.endTransaction()

require 'img.sprites'
