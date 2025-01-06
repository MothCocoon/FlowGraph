## Concept
Flow plug-in for Unreal Engine is a design-agnostic event node editor. It provides a graph editor tailored for scripting flow of events in virtual worlds. It's based on a decade of experience with designing and implementing narrative layer in video games. All we need here is simplicity.

The aim of publishing it as an open-source project is to let people tell great stories and construct immersive worlds more easily. That allows us to enrich video game storytelling so we can inspire people and make our world a better place.

![Flow101](https://user-images.githubusercontent.com/5065057/103543817-6d924080-4e9f-11eb-87d9-15ab092c3875.png)

* A single node in this graph is a simple UObject, not a function like in blueprints. This allows you to encapsulate the entire gameplay element (logic with its data) within a single Flow Node. The idea is that you write a repeatable "event script" only once for the entire game!
* Unlike blueprints, Flow Node is async/latent by design. Active nodes usually subscribe to delegates, so they can react to events by triggering output pins (or whatever you choose to).
* Every node defines its own set of input/output pins. It's dead simple to design the flow of the game - just connect nodes representing features.
* Developers creating a Flow Node can call the execution of pins any way they need. API is extremely simple.
* Editor supports convenient displaying debug information on nodes and wires while playing a game. You simply provide what kind of message would be displayed over active Flow Nodes - you can't have that with blueprint functions.
* It's up to you to add game-specific functionalities by writing your nodes and editor customizations. It's not like a marketplace providing very specific implementation of systems. It's a convenient base for building systems tailored to fit your needs.
* Please visit the wiki. I'd recommend starting by reading about [design philosophy](https://github.com/MothCocoon/FlowGraph/wiki). Also, check useful pages listed on the sidebar.
* It's easy to include a Flow plugin in your own project, follow this short [Getting Started](https://github.com/MothCocoon/FlowGraph/wiki/Getting-Started) guide.

## In-depth video presentation
This 24-minute presentation breaks down the concept of the Flow Graph. Trust me, you want to understand concept properly before diving into implementation.

[![Introducing Flow Graph for Unreal Engine](https://img.youtube.com/vi/BAqhccgKx_k/0.jpg)](https://www.youtube.com/watch?v=BAqhccgKx_k)

## Acknowledgements
I got an opportunity to work on something like the Flow Graph at Reikon Games. They shared my enthusiasm for providing the plugin as open source and as such allowed me to publish this work and keep expanding it as a personal project. Kudos, guys!
Reikon badly wanted to build a better tool for implementing game flow rather than level blueprints or existing Marketplace plug-ins. I was very much interested in this since the studio was just starting with the production of a new title. And we did exactly that, created a node editor dedicated to scripting game flow. Kudos to Dariusz Murawski - a programmer who spent a few months with me to establish the working system and editor. And who had to endure my never-ending feedback and requests.

I feel it's important to mention that I didn't invent anything new here, with the Flow Graph. It's an old and proven concept. I'm just one of many developers who decided it would be crazy useful to adopt it for Unreal Engine. This time, also to make it publicly available as an open-source project.
* Such simple graph-based tools for scripting game screenplay have been utilized for a long time. Traditionally, RPG games needed such tools as there are a lot of stories, quests, and dialogues.
* The best narrative toolset I had the opportunity to work with is what CD Projekt RED built for The Witcher series. Sadly, you can't download the modding toolkit for The Witcher 2 - yeah, it was publically available for some time. Still... you can watch the GDC talk by Piotr Tomsiński on [Cinematic Dialogues in The Witcher 3: Wild Hunt](https://www.youtube.com/watch?v=chf3REzAjgI) - it includes a brief presentation how Quest and Dialogue editors look like. It wouldn't be possible to create such an amazing narrative game without this kind of toolset. I did miss that so much when I moved to the Unreal Engine...
* At some point I felt comfortable enough with programming editor tools so I decided to build my own version of such toolset, meant to be published as an open-source project. I am thankful to Reikon bosses they see no issues with me releasing Flow Graph, which is "obviously" similar to our internal tool in many ways. I mean, it's so simple concept of "single node representing a single game feature"... and it's based on the same UE4 node graph API. Some corporations might have an issue with that.
