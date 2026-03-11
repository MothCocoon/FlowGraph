---
title: Adding Node Spawn Shortcut
---

1. Create `DefaultEditorPerProjectUserSettings.ini` in your project's Config folder, if you don't have this .ini yet.
2.  Add this section to the .ini
```
[FlowSpawnNodes]
; Flow
+Node=(Class=FlowNode_OnNotifyFromActor Key=A Shift=false Ctrl=false Alt=false)
+Node=(Class=FlowNode_Finish Key=F Shift=false Ctrl=false Alt=false)
+Node=(Class=FlowNode_SubGraph Key=G Shift=false Ctrl=false Alt=false)
+Node=(Class=FlowNode_CustomInput Key=I Shift=false Ctrl=false Alt=false)
+Node=(Class=FlowNode_Log Key=L Shift=false Ctrl=false Alt=false)
+Node=(Class=FlowNode_ExecutionMultiGate Key=M Shift=false Ctrl=false Alt=false)
+Node=(Class=FlowNode_NotifyActor Key=N Shift=false Ctrl=false Alt=false)
+Node=(Class=FlowNode_CustomOutput Key=O Shift=false Ctrl=false Alt=false)
+Node=(Class=FlowNode_Reroute Key=R Shift=false Ctrl=false Alt=false)
+Node=(Class=FlowNode_ExecutionSequence Key=S Shift=false Ctrl=false Alt=false)
+Node=(Class=FlowNode_Timer Key=T Shift=false Ctrl=false Alt=false)
; Comment
+Node=(Name=Comment Key=C Shift=false Ctrl=false Alt=false)
```
3. Restart the editor. You should see the assigned shortcut in the Palette.

![BKCt3pDj9s](https://user-images.githubusercontent.com/5065057/114264581-062ee400-99ec-11eb-9ffc-ba3e6d901b3d.png)
