# Flow

Flow plug-in for Unreal Engine provides a graph editor tailored for scripting flow of events in virtual worlds. It's based on a decade of experience with designing and implementing narrative in video games. All we need here is simplicity.
* Licensed under MIT license. You are free to use it for commercial projects, modify it however you see fit, and distribute it further.

## Concept
It's s design-agnostic event node editor.

![Flow101](https://user-images.githubusercontent.com/5065057/103543817-6d924080-4e9f-11eb-87d9-15ab092c3875.png)

* A single node in this graph is a simple UObject, not a function like in blueprints. This allows you to encapsulate the entire gameplay element (logic with its data) within a single Flow Node. The idea is that your write a repeatable "event script" only once for the entire game!
* Unlike blueprints, Flow Node is async/latent by design. Active node usually subscribe to delegates, so it can react to event by triggering output pin (or whatever you choose to).
* Every node defines its own set of input/output pins. It's dead simple to design the flow of the game - just connect nodes representing features.
* Developers creating a Flow Node can call the execution of pins any way they need. API is extremely simple.
* Editor supports convenient displaying debug information on nodes and wires while playing a game. You simply provide what kind of message would be displayed over active Flow Nodes - you can't have that with blueprint functions.

## Base for your own systems and tools
* It's up to you to add game-specific functionalities by writing your nodes and editor customizations. It's not like a marketplace providing the very specific implementation of systems. It's a convenient base for building systems tailored to fit your needs.
* Quickly build your own Quest system, Dialogue system or any other custom system that would control the flow of events in the game.
* Expand it, build Articy:draft equivalent right in the Unreal Engine.

## Getting started
In Releases, you can find an example project called Flower, so you can easily check how this plug-in works.

And if you'd decide to use Flow in your project...
1. Unpack plug-in to the Plugins folder in your project folder. If you don't have such a folder yet, simply create it.
2. Open Project Settings in the editor. Change World Settings to the Flow World Settings class and restart the editor. This class starts the Flow Graph assigned to the map.

## In-depth video presentation
This 24-minute presentation breaks down the concept of the Flow Graph. It goes through everything written in this ReadMe but in greater detail.

[![Introducing Flow Graph for Unreal Engine](https://img.youtube.com/vi/Rj76JP1f-I4/0.jpg)](https://www.youtube.com/watch?v=Rj76JP1f-I4)

## Simplicity is a key
* It's all about simplifying the cooperation between gameplay programmers and content designers by providing a clean interface between "code of systems" and "using systems".
* Code of gameplay mechanics wouldn't ever be mixed with each other. Usually, system X shouldn't even know about the existence of system Y. Flow Graph is a place to combine features by connecting nodes.
* Every mechanic is exposed to content designers once, in one way only - as the Flow Node. It greatly reduces the number of bugs. Refactoring mechanics is easy since you don't have to update dozens of level blueprints directly calling system functions.
* Systems based on such editor are simple to use for least technical team members, i.e. narrative designers, writers, QA. Every time I ask designers why they love working with such a system, they usually reply: "it's so simple to understand and make a game with it".
* Even a complex game might end up only with a few dozens of Flow Nodes. It's easy to manage the game's complexity - a lot of mechanics, mission scripting, narrative events. It makes it very efficient to develop lengthy campaigns and multiplayer games.

## Blueprints
* Programmer writing a new gameplay feature can quickly expose it to content creators by creating a new Flow Node. A given C++ feature doesn't have to be exposed to blueprints at all.
* However, Flow Nodes can be created in blueprints by anyone. Personally, I would recommend using blueprint nodes mostly for prototyping and rarely used custom actions, if you have a gameplay programmer in a team. If not, sure, you can implement your systems in blueprints entirely.

## Performance
* Performance loss in blueprint graphs comes from executing a large network of nodes, processing pins and connections between them. Moving away from overcomplicated level blueprints and messy "system blueprints" to simple Flow Graph might improve framerate and memory management. 
* As Flow Nodes are designed to be event-based, executing graph connection might happen only like few times per minute or so. (heavily depends on your logic and event mechanics). Finally, Flow Graph has its own execution logic, doesn't utilize blueprint VM.
* Flow-based event systems are generally more performant than blueprint counterparts. Especially if frequently used nodes are implemented in C++.

## Flexibility of the system design
Flow Graph communicates with actors in the world by using [Gameplay Tags](https://docs.unrealengine.com/en-US/Gameplay/Tags/index.html). No direct references to actors are used in this variant of scripting - that brings a lot of new possibilities.
* Simply add Flow Component to every "event actor", assign Gameplay Tags identifying this actor. Flow Component registers itself to the Flow Subsystem (or any derived system) when it appears in the world. It's easy to find any event actor this way, just ask Flow Subsystem for actors registered with a given Gameplay Tag.
* It allows for reusing entire Flow Graphs in different maps. Unlike level blueprints, Flow Graphs aren't bound to levels.
* It's possible to place actors used by the single Flow Graph in different sublevels or even worlds. This removes one of the workflow limitations related to the level design.
* It should work well with the World Partition system coming to Unreal Engine - a truly open-world way of building maps where every actor instance is saved separately to disk. That probably means no sublevels and no level blueprints (except the blueprint of the "persistent level").
* Flow Graph could live as long as the game session, not even bound to the specific world. You can have a meta Flow Graph waiting for events happening anywhere during the game.
* Using Gameplay Tags allows scripting an action on any actor spawned in runtime, typically NPCs.
* In some cases actor with a given Gameplay Tag doesn't even have to exist when starting a related action! Example: On Trigger Enter in the image above would pick up the required trigger after loading a sublevel with this trigger.

## Recommended workflow
* Flow Graph is meant to entirely replace a need to use Level Blueprints (also known as Flying Spaghetti Monster) in production maps. The flow of the game - the connection between consecutive events and actors - should be scripted by using Flow Graphs only. Otherwise, you ending up creating a mess, using multiple tools for the same job.
* This graph also entirely replaces another way of doing things: referencing different actors directly, i.e. hooking up Spawner actor directly to the Trigger actor. Technically it works fine, but it's impossible to read the designed flow of events scripted this way. Debugging can be very cumbersome and time-consuming.
* Actor blueprints are supposed to be used only to script the inner logic of actors, not connections between actors belonging to different systems.
* Flow Nodes can send and receive blueprint events via Flow Component. This recommended way of communicating between Flow Graph and blueprints.
* Technically, it's always possible to call custom blueprint events directly from blueprint Flow Node, but this would require creating a new Flow Node for every custom blueprint actor. Effectively, you would throw a simplicity of Flow Graph out of the window.

## State of the development
* Flow editor and runtime system are ready for production.
* Plugin and the code of the sample project are available for every engine version since 4.22. I'm not planning on backporting code to older engine versions since the runtime code relies on [programming subsystems](https://docs.unrealengine.com/en-US/ProgrammingAndScripting/Subsystems/index.html) introduced with UE 4.22.
* Currently, the example content is... modest. I hope to prepare better samples in the future.
* Development continues. Check [Issues](https://github.com/MothCocoon/Flow/issues) for a list of useful things I'm hoping to work on in the future.
* Your feedback is much welcome! It's all about developing a toolset for any kind of game.
* In the short term, code might need a bit of love to support creating multiple flow-based systems without modifying the plugin code at all. For example, the quest and dialogue system based on the Flow Subsystem. It's not a huge work, plugin was designed for it. I just need time to create a few different systems, play with it, update the plugin where's needed. 
* I'm planning to release the Flow plugin on the Marketplace, so more people could discover it and conveniently add it to their asset libraries. It will be free of charge, obviously.

## Contact
* Catch me on Twitter: [@MothDoctor](https://twitter.com/MothDoctor)
* Discuss thins related to plugin on the Discord server named Flow.

## Acknowledgements
I feel it's important to mention that I didn't invent anything new here, with the Flow Graph. It's an old and proven concept. I'm only the one who decided it would be crazy useful to adopt it for Unreal Engine. And make it publically available as my humble contribution to the open-source community.
* Such simple graph-based tools for scripting game screenplay are utilized for a long time. Traditionally, RPG games needed such tools as there a lot of stories, quests, dialogues.
* The best narrative toolset I had the opportunity to work with is what CD Projekt RED built for The Witcher series. Sadly, you can't download the modding toolkit for The Witcher 2 - yeah, it was publically available for some time. Still... you can watch the GDC talk by Piotr Tomsiński on [Cinematic Dialogues in The Witcher 3: Wild Hunt](https://www.youtube.com/watch?v=chf3REzAjgI) - it includes a brief presentation how Quest and Dialogue editors look like. It wouldn't be possible to create such an amazing narrative game without this kind of toolset. I did miss that so much when I moved to the Unreal Engine...
* Finally got an opportunity to work on something like this at [Reikon Games](http://www.reikongames.com/). They badly wanted to build a better tool for implementing game flow than level blueprints or existing Marketplace plug-ins. I was very much interested in this since the studio was just starting with the production of the new title. And we did exactly that, created a node editor dedicated to scripting game flow. Kudos to Dariusz Murawski - a programmer who spent a few months with me to establish the working system and editor. And who had to endure my never-ending feedback and requests.
* At some point I felt comfortable enough with programming editor tools so I decided to build my own version of such toolset. Written from the scratch, meant to be published as an open-source project. I am thankful to Reikon bosses they see no issues with me releasing Flow Graph, which is "obviously" similar to our internal tool in many ways. I mean, it's so simple concept of "single node representing a single game feature"... and it's based on the same UE4 node graph API. Some corporations might have an issue with that.

## Related resources
* [Introduction to Gameplay Tags](https://docs.unrealengine.com/en-US/ProgrammingAndScripting/Tags/index.html)
* [Behind the Scenes of the Cinematic Dialogues in The Witcher 3: Wild Hunt](https://www.youtube.com/watch?v=chf3REzAjgI)
* [Sinking City - story scripting for the open-world game](https://youtu.be/W_yiopwoXt0?t=929) as part of their talk on Sinking City development
* [Unreal Engine Open World Preview and Landscape Tools | Unreal Dev Days 2019](https://www.youtube.com/watch?v=wXbyqGYfM1I) - starts from describing the upcoming open-world streaming system that might no longer use level blueprints
* [Blueprints In-depth - Part 1 | Unreal Fest Europe 2019](https://youtu.be/j6mskTgL7kU?t=1048) - great talk on blueprint system, the timestamp at the Performance part.
* [Blueprints In-depth - Part 2 | Unreal Fest Europe 2019](https://www.youtube.com/watch?v=0YMS2wnykbc)
* [The Visual Logger: For All Your Gameplay Needs!](https://www.youtube.com/watch?v=hWpbco3F4L4)
* [Gamedec exemplifies how to incorporate complex branching pathways using Unreal Engine](https://www.unrealengine.com/en-US/tech-blog/gamedec-exemplifies-how-to-incorporate-complex-branching-pathways-using-unreal-engine) - example how the integration of Artict:Draft with Unreal Engine looks like
