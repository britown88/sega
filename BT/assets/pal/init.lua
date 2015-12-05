palette = palette or {}
palette.directory = "assets/pal/"

function loadPalette(fname)
  palette.load(string.format("%s%s.pal", palette.directory, fname))
  console.print(string.format("Loaded palette: [c=0,5]%s[/c]", fname))
end
