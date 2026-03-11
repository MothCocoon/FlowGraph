---
title: FAQ
---

## How to separate the logic between many graphs?
The basic and recommended way is to use the `Sub Graph` node to start graph B from graph A. This way we could have very complex logic, i.e. separate graphs for every quest or event. 

![image](https://user-images.githubusercontent.com/5065057/204156931-8f21eb1a-d438-4430-8e8a-f1365e4528ea.png)

## Is it possible to have a Flow Graph independent from the world?
Yes, it's an everyday use case, i.e. if you want to have a global "meta-quest" (controlling the game's story) or graph that is tracking achievements. You can create a Flow Asset instance from anywhere in the game by calling `UFlowSubsystem::StartRootFlow`.

![image](https://user-images.githubusercontent.com/5065057/150850841-3827e785-dd71-4c08-9ab7-aa1a94631052.png)

## My Flow Node blueprint isn't visible in the Palette?
This happens if a given blueprint was created with a standard blueprint creation menu. You have to create blueprint nodes via a dedicated Content Browser menu item. (Similar inconvenience applies to creating Editor Blutilites, which won't run if created via the standard blueprint menu).

![image](https://user-images.githubusercontent.com/5065057/204156900-87fe32cd-daa9-4bd4-9e7f-a42ead0d0443.png)
