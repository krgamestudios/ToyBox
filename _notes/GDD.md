# GDD

# Concept & Overview

This is an RTS game where the players write code to automate their forces, without having direct input while the game is running. It has elements of auto battlers, and draws inspiration from games like "Screeps" and "Starcraft".

# Gameplay

The gameplay takes place in a grid-based world with various procedurally generated elements like walls and corridors. A player's forces must hunt down and destroy the oppoising player's core before they can do the same in return. To this end, players write algorithms using the bespoke scripting language "Toy" and API which is executed during each game "tick". How they design their algorithms will greatly impact the effectiveness of their forces.

# Mechanics

A list of game objects/concepts players can control or fight for:

* Core - the central hub of a player's forces, and the target for other players
* Creeps - moveable units that can collect resources and fight other player's units
* Source - Part of the terrain, used to produce energy
* Energy - Consumed to produce new creeps, and other elements of the game

# Scripting API

For the time being, see `initEngineAPI` and `initGameAPI` for the APIs provided to `setup.toy` and the players, respectfully.

See [toylang.com/](https://toylang.com/) for a short summary of the Toy langauge.

# Graphical & Audio Assets

Any graphics and audio will be simple programmer art for the time being - what matters is the game is readable rather than pretty.