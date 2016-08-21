#include "AssetHelpers.h"
#include "segalib/EGA.h"
#include "SEGA/App.h"

void assetsSetPalette(DB_assets *db, StringView id) {
   DBPalette pal = dbPaletteSelectFirstByid(db, id);

   if (pal.id) {
      appSetPalette(appGet(), (Palette*)pal.palette);
      dbPaletteDestroy(&pal);
   }
}