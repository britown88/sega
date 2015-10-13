# sEGA
The "Super EGA" C Game Engine Project

## What is it?
The goal of the project is to create a full-featured modern game engine entirely in C which emulates the [EGA Graphics Format](http://en.wikipedia.org/wiki/Enhanced_Graphics_Adapter) for rendering to the screen.  C has a wealth of quirky hacky things you can do that would be difficult in more verbose and strict languages so the "academic" goal of the project is to break some of the assumptions about C being unweildy.

What started as a small graphics demo rendering 16 colors from a palette to bitplanes quickly turned into a whole library of EGA functions including the ability to convert PNG images to the EGA format by generating an optimal palette from the EGA color space to use.

## How do I build it?
[Visual Studio Community 2015 is free now!](https://www.visualstudio.com/en-us/products/vs-2015-product-editions.aspx)  I feel like not using cmake-gcc-cross-platform-super-indie-python-something-makefile-build-pattern© is a bit of a taboo and so sorry for tarnishing github's fine image with this corporate trash but really, VS2015 has some really wonderful git integration and it's been a joy to work with.

Currently in the solution there is Sega Studio, a .NET app for creating EGA image files, and two game projects, BT and Shift.  Everything uses the segalib library for EGA functions.  If you want to use segalib, make sure to edit the config.h file to set which video mode you wish to compile it with.  BT uses 320x200 and Shift uses 640x480!

## What is Sega Studio?
A quick and dirty .Net App that uses C++/CLI to call my EGA library functions.  You can open PNG files, create a full or partial palette to render it with, and then convert to EGA which will find the optimal way to encode the image with those colors.  While in EGA edit mode you can also make palette changes and it will show the result in the viewer as you hover over the colors making it easy to test out palette-swaps!

If the app crashes loading certain PNG files it's probably due to the PNG not being RGB, which you should change using Gimp or some other image editor.  I should really fix imagetools.h to make other formats work...

## Fun C Things
I've enjoyed implementing some C++ niceties into C including std::unordered_map, std::vector, std::function (!), and even a priority quuee that uses an intrusive pairing heap which I use for my dijkstras solves.  Most of these are in the segautils library so take a look!

## EGA Emulator?
Rendering only allows a pixel resolution of 640x350 and each pixel is a 4-bit index into a 16-color palette which contains indices into a 64-color range.  These restrictions arent just arbitrarily enforced, the render pipeline actually uses bitplanes and bitwise operations to and/or color data to the frame buffer.  This adds in interesting restrictions for games.  Just like the "Software Authors" of old who made several indie titles with the format long after it fell out of commercial use, the users of this engine will need to be mindful of the scene's palette and design their image files to use the right palette indices appropriately (Sega Studio is hugely helpful for this).

Text rendering is no different and uses a single font on a set text grid to emulate the text mode of the format and so background and UI needs to be designed with the grid in mind so that dynamic text will line up with UI Elements!

## Questions?
You can shoot me an email at p4r4digm@gmail.com or tweet at @p4r4digm




