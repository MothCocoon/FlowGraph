---
title: Concept
---

Flow plug-in for Unreal Engine provides a graph editor tailored for scripting flow of events in virtual worlds. It's based on a decade of experience with designing and implementing narrative in video games. All we need here is simplicity.

It's a design-agnostic event node editor.

![Flow101](https://user-images.githubusercontent.com/5065057/103543817-6d924080-4e9f-11eb-87d9-15ab092c3875.png)

* A single node in this graph is a simple UObject, not a function like in blueprints. This allows you to encapsulate the entire gameplay element (logic with its data) within a single Flow Node. The idea is to write a reusable "event script" only once for the entire game!
* Unlike blueprints, Flow Node is async/latent by design. Active nodes usually subscribe to delegates, so they can react to an event by triggering the output pin (or whatever you choose).
* Every node defines its own set of input/output pins. It's dead simple to design the flow of the game - connect nodes representing features.
* Developers creating a Flow Node can call the execution of pins in any way they need. API is extremely simple.
* Editor supports conveniently displaying debug information on nodes and wires while playing a game. You provide what kind of message would be displayed over active Flow Nodes. You can't have that with blueprint functions.
* Works well with the World Partition introduced with UE 5.0. In thic case, there are no sublevels and no level blueprints.

## Base for your own systems and tools
* It's up to you to add game-specific functionalities by writing your nodes and editor customizations. It's not like a marketplace providing the very specific implementation of systems. It's a convenient base for building systems tailored to fit your needs.
* Quickly build your own Quest system, Dialogue system, or any other custom system that would control the flow of events in the game.
* Expand it, build Articy:Draft equivalent right in the Unreal Engine.

## In-depth video presentation
This 24-minute presentation breaks down the concept of the Flow Graph. It goes through everything written here, but in greater detail.

<a href="https://www.youtube.com/watch?v=BAqhccgKx_k"><img src="https://img.youtube.com/vi/BAqhccgKx_k/0.jpg" alt="Introducing Flow Graph for Unreal Engine"></a>

## Simplicity is a key
* It's all about simplifying the cooperation between gameplay programmers and content designers by providing a clean interface between "code of systems" and "using systems".
* The code of gameplay mechanics wouldn't ever be mixed. Usually, system X shouldn't even know about the existence of system Y. Flow Graph is a place to combine features by connecting nodes.
* Every mechanic is exposed to content designers once, in one way only - as the Flow Node. It greatly reduces the number of bugs. Refactoring mechanics is easy since you don't have to update dozens of level blueprints by directly calling system functions.
* Systems based on such an editor are simple to use for the least technical team members, like narrative designers, writers, QA. Every time I ask designers why they love working with such a system, they usually reply: "It's so simple to understand and make a game with it".
* Even a complex game might end up with a few dozen Flow Nodes. It's easy to manage the game's complexity - a lot of mechanics, mission scripting, narrative events. It makes it very efficient to develop lengthy campaigns and multiplayer games.

## Flexibility of the system design
Flow Graph assets aren't part of the world. Every Flow Node can communicate with world actors any way you see fit.
* Nodes can obtain actors in the world by using [Gameplay Tags](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine). No direct references to actors are used in this variant of scripting. That brings a lot of new possibilities.
    * Simply add a Flow Component to every "event actor", and assign Gameplay Tags identifying this actor. Flow Component registers itself with the Flow Subsystem (or any derived system) when it appears in the world. It's easy to find any event actor this way, just ask the Flow Subsystem for actors registered with a given Gameplay Tag.
    * This is the best way of identifying on runtime-spawner actors, especially NPCs.
    * In some cases actor with a given Gameplay Tag doesn't even have to exist when starting a related action! Example: On Trigger Enter in the image above would pick up the required trigger after loading a sublevel with this trigger.
* Nodes can obtain actors in other way like Soft Object Reference or Guid. This is often the preferred way of referencing unique triggers or spawn points.

Thanks to this flexibility, it's possible to place actors used by the single Flow Graph in different sublevels or even worlds. This removes one of the workflow limitations related to the level design.

Flow Graph could live as long as the game session, not even bound to a specific world. You can have a meta Flow Graph waiting for events happening anywhere during the game.

## Healthy architecture
* Flow Graph is meant to entirely replace the need to use Level Blueprints in production maps. The flow of the game - the connection between consecutive events and actors - should be scripted by using Flow Graphs only. Otherwise, you end up creating a mess, using multiple tools for the same job.
* This graph also entirely replaces another way of doing things: referencing different actors directly, i.e. hooking up Spawner actor directly to the Trigger actor. This seemingly works fine, but it's impossible to read the designed flow of events scripted this way. Debugging can be very cumbersome and time-consuming.
* Actor blueprints are supposed to be used only to script the inner logic of actors, not connections between actors belonging to different systems.
* Flow Nodes can send and receive blueprint events via the Flow Component. This recommended way of communicating between Flow Graphs and blueprints.
* Technically, it's always possible to call custom blueprint events directly from a blueprint Flow Node, but this would require creating a new Flow Node for every custom blueprint actor. Effectively, you would throw the simplicity of Flow Graph out of the window.

## C++ vs Blueprints
* A programmer writing a new gameplay feature can quickly expose it to content creators by creating a new Flow Node. A given C++ feature doesn't have to be exposed to blueprints at all.
* Flow Nodes can be created in blueprints as well. Personally, I would recommend using blueprint nodes mostly for prototyping and rarely used custom actions, if you have a gameplay programmer in your team. If not, sure, you can implement your systems in blueprints entirely.

## Performance
* Performance loss in blueprint graphs comes from executing a large network of nodes, processing pins and connections between them. Moving away from overcomplicated level blueprints and messy "system blueprints" to simple Flow Graphs might improve framerate and memory management. 
* As Flow Nodes are designed to be event-based, executing graph connections might happen only a few times per minute or so. (heavily depends on your logic and event mechanics). Finally, Flow Graph has its own execution logic, doesn't utilize blueprint VM.
* Flow-based event systems are generally more performant than blueprint counterparts. Especially if frequently used nodes are implemented in C++.

## Related resources
* [Introduction to Gameplay Tags](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-tags-in-unreal-engine)
* [Behind the Scenes of the Cinematic Dialogues in The Witcher 3: Wild Hunt](https://www.youtube.com/watch?v=chf3REzAjgI)
* [Story of Choices: the quest system in Dying Light 2](https://www.youtube.com/watch?v=DPcz_-m3SwQ)
* [Sinking City - story scripting for the open-world game](https://youtu.be/W_yiopwoXt0?t=929) as part of their talk on Sinking City development.
* [Large worlds in UE5: A whole new (open) world](https://www.youtube.com/watch?v=ZxJ5DG8Ytog) - describes World Partition and related features.
* [Blueprints In-depth - Part 1](https://youtu.be/j6mskTgL7kU?t=1048) - great talk on blueprint system, the timestamp at the Performance part.
* [Blueprints In-depth - Part 2](https://www.youtube.com/watch?v=0YMS2wnykbc)
* [The Visual Logger: For All Your Gameplay Needs!](https://www.youtube.com/watch?v=hWpbco3F4L4)
* [Gamedec exemplifies how to incorporate complex branching pathways using Unreal Engine](https://www.unrealengine.com/en-US/tech-blog/gamedec-exemplifies-how-to-incorporate-complex-branching-pathways-using-unreal-engine) - example of how the integration of Articy:Draft with Unreal Engine looks like.
