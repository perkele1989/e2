# e2

If you're reading this, you're looking at one of my hobby projects that will probably never be finished or released. This is made public for educational purposes only, I reserve all rights to this project.

e2 refers to "engine2", as this is my second game engine. 

This repository is _very messy_ and I would recommend you do not build anything even resembling production code with this.

## The engine (e2.core, e2.editor)

* Full build system and package manager written in python, with static code generation for reflection
* RHI with Vulkan implementation
* A somewhat general-purpose forward renderer, with support for custom shader models (both native shader models written in C++, but also data driven loadable from json)
* Skeletal animation support
* An immediate-mode UI framework, on top of which a very janky editor exists
* A ton of prototype code that does nothing

## A game (e2.game)

Started as a 4x game in the spirit of Civilization VI. Later refurbished to a tower defense game. Later refurbished into it's current iteration; A top down valheim-style game, with a bunch of cool random things implemented.

* Infinite procedural worlds, generated in a hex-grid
* Electrical circuitry game mechanic, fully working and integrated with the item/equipment system
* Pretty cool grass simulations, and grass mowing a' la Zelda
* Very expensive procedural water shader, including water physics (and boatin')
* Quest and Dialogue system
* Data-driven Entity-Component system via json
* Item and equipment system
* Tree-chopping
* Simple combat system and enemies
* Two kinds of bombs, can be used to trigger chain events
* "Minimap" and fog of war
* Probably more stuff

## A demonstration application for a Youtube video I'm making (e2.demo)

* Features a neat water shader and visualizations of steps required to implement it 
