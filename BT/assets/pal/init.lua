pal = pal or {}
pal.directory = "assets/pal/"

function loadPal(fname)
  pal.load(string.format("%s%s.pal", pal.directory, fname))
  console.print(string.format("Loaded palette: [c=0,5]%s[/c]", fname))
end
