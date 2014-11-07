#pragma once

#define CONCAT2(x, y)      x##y
#define CONCAT(x, y)       CONCAT2(x, y)

#define XSTRINGIFY(x) #x
#define STRINGIFY(x) XSTRINGIFY(x)