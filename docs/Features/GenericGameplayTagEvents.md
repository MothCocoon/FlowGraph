---
title: Generic Gameplay Tag Events
---

Nodes available in the Flow plugin support finding actors and communicating with them by Gameplay Tags. 
- This can have many advantages over using soft references to actor instances, i.e. changing actor instance name won't break reference.
- This also allows communicating with actors spawned in runtime, i.e. all characters.
- You can call multiple actors identified by the same Gameplay Tag, which may be useful in many cases.

Obviously, you can use soft references to actors in your project-specific Flow nodes. Although the Flow plugin comes with a mechanism to call events in a generic way, without the need to know the actor's class and functions.

## Adding Flow Component to Actor

* Add `Flow Component` to the actor.
* Assign a unique gameplay tag on the `Identity Tags` list.

![image](https://user-images.githubusercontent.com/5065057/176913188-077dc82a-2c7d-4af1-ac30-42d7fba869d5.png)

## Calling Actor events from the Flow Graph

* Add the `Receive Notify` event from the Flow Component.

![image](https://user-images.githubusercontent.com/5065057/176913685-180f9f6c-a17e-4097-8e8f-2984a89efaae.png)
* Place the `Notify Actor` node in your Flow Graph.
  * `Identity Tags` should include the Identity Tag assigned in the Flow Component.
  * `Notify Tag` is an optional tag that would be sent through the `Receive Notify` event to your actor. It allows you to call different events in the same actor.

![image](https://user-images.githubusercontent.com/5065057/176920012-98c34c6c-fa7c-43b7-a5ce-be928eae24a3.png)

## Calling Flow Graph event from the Actor

* Call `Notify Graph` from the Flow Component.

![image](https://user-images.githubusercontent.com/5065057/176914462-d91f43a3-722d-4c77-bc83-48ee69d0525b.png)
* Place the `On Notify From Actor` node in your Flow Graph.
  * `Identity Tags` should include the Identity Tag assigned in the Flow Component.
  * `Notify Tag` is optional. If added, it needs to match the tag selected on Notify Graph in the step above. Otherwise, the `Success` output won't be triggered.
  * If `Notify Tag` will be empty, the Notify node won't check what tag has been sent from the actor. The `Success` output will always be triggered.

![image](https://user-images.githubusercontent.com/5065057/176920480-9341ac92-b96d-448d-976a-5590e14ae20b.png)

## Calling events between Actors

* Source Actor: call `Notify Actor` from the Flow Component.
  * `Notify Tag` is optional. It allows you to call different events in the target actor.

![image](https://user-images.githubusercontent.com/5065057/176920761-d7f10c83-e11f-4955-85d5-3caf70f20c49.png)
* Target Actor: add the `Receive Notify` event from the Flow Component.

![image](https://user-images.githubusercontent.com/5065057/176921316-bc021dcc-a024-4a56-923e-c2a19889c58e.png)

## Calling Flow Graph events from Sequencer

* Add the `Flow Events` track to the Level Sequence.
  * Simply place a key on the section and give it a name.

![image](https://user-images.githubusercontent.com/5065057/176922388-60e42447-a6aa-4cd8-a67d-5e2ac1be5280.png)

* Include this Level Sequence on any `Play Level Sequence` node.
  * Either use the `Refresh Asset` button on the toolbar or the `Refresh Context Pins` option on the node. You need to use it after every change of event names, adding or removing events from the timeline.

![image](https://user-images.githubusercontent.com/5065057/176922906-a0a8b327-8227-4ba0-a088-0e1d33370d2d.png)

![image](https://user-images.githubusercontent.com/5065057/176923030-1d906f00-44fa-4812-8e04-2b8550828aff.png)

