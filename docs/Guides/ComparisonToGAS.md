---
title: Comparison to GAS
---

Some people asked how the Flow system differs from systems like Gameplay Ability System (GAS)? Naturally, this might be unclear if someone used to implement gameplay mechanics, but not event systems like quests.

Flow and GAS are designed for different groups of designers, working on separate layers of gameplay.

### Ability System is designed to script second-to-second gameplay. A core gameplay loop.
* It's typically used by gameplay-heavy games, where designers need to implement a lot of different skills, attacks/spells, buffs/debuffs. All different kinds of gameplay abilities and effects. GAS provides an abstract framework for this kind of gameplay.
* **GAS is a tool for gameplay designers.** Developers that script game mechanics in blueprints. If we'd have a scripting language in the engine, they might spend most of their time implementing features in such scripting language (instead of Blueprint or C++).
* Execution of abilities depends heavily on the player's actions and AI behaviors. Abilities are conceptually related to Pawns and Player/AI Controllers. Usually, the player character and enemy character would use abilities against each other.
* It's player-driven and AI-driven. Players decide what skill, weapon, or attack use at the given moment. GAS doesn't use any kind of global graph defining what should happen in the game. It provides methods to manage complex interactions between actions, i.e. casting spell X on an enemy would interrupt his attack Z (which is another ability).
* GAS allows for scripting combat/action mechanics in blueprints but in a very systemic and organized way. It's probably the only kind of blueprint which supports efficient network replication (i.e. optimal replication of arrays which is normally only available in C++).

### The Flow system is designed to easily script pre-designed minute-to-minute gameplay. Event-heavy games.
* It's used for directing what exactly happens in the given place, in the specific moments of the storyline. Events happen in specifically designed order, in predictable cause-effect chains.
* **Flow is a tool for content designers.** People who usually don't focus on scripting gameplay systems. Content designers craft world and story. Things that the player explores while repeating the core gameplay loop. The flow-based system should be easy to use by the least technical designers, as writers.
* In the perfect world, a Flow-based graph for the quest would resemble a whiteboard graph drawn while the quest was designed. It should be easier to translate story/event design documents to the Flow Graph than any other visual scripting available for the Unreal Engine. 
* It's important to have easy-to-maintain graphs telling the entire story of the game. It's your way to script a "walkthrough of the game".
* If there's at least one gameplay programmer in your team, he might implement all the systems and Flow Nodes in C++. It wouldn't take away anything from content designers if we assume they only want to build a game from reusable "event blocks".

### Example: implementing the Starcraft campaign.
* You might use Ability System to do script skills of all units. Also how much resources they consume. What units can be produced in a given building, etc.
* You might use Flow to script specific missions. For instance, the first objective is to destroy 3 buildings on the map. Once player units enter trigger X for the first time, we play a video message from the command center. After destroying these 3 buildings, we give players another objective like "destroy all enemy constructions".
* You might use Flow graphs to script tutorials activated under specific conditions. "If the Coal resource is depleted for the first time, start tutorial explaining how to build a coal mine".

Obviously, Flow Graph can be used to script many different things. It's basically a generalized way for triggering any kind of actions and reacting to dispatched events (brodcasted delegates). It might be used to implement gameplay mechanics, especially if the gameplay is mixed with the narrative (like puzzles).
