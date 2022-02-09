Scriptname SAF Native Hidden

Struct Transform
    string name
    bool override
    float x
    float y
    float z
    float roll
    float pitch
    float yaw
    float scale
EndStruct

; This creates a new adjustment on the reference. 
; The integer returned by this function is used as a handle to interact with this specific adjustment
; You have to specificy your mod's .esp/.esl filename so it can erase the adjustment if the mod isn't found
; Persistent saves the adjustment to the co-save
; Hidden hides your adjustment from other mods
; eg. CreateAdjustment(ref, "Sam Adjustment", "ScreenArcherMenu.esp", false, true)
int Function CreateAdjustment(ObjectReference akRef, string name, string espName, bool persistent, bool hidden) native global

; Checks if the adjustment for the handle exists
bool Function HasAdjustment(ObjectReference akRef, int handle) native global

; Removes the adjustment
Function RemoveAdjustment(ObjectReference akRef, int handle) native global

; Returns the name of the adjustment 
string Function GetAdjustmentName(ObjectReference akRef, int handle) native global

; Gets all adjustment handles owned by your mod, Use this to get persistent handles.
; Handle values may be different each load.
; If using more than 1 adjustment use getAdjustmentName to determine which one it is.
int[] Function GetAdjustments(ObjectReference akRef, string espName) native global

; Modifies your adjustments persistence
Function SetAdjustmentPersistence(ObjectReference akRef, int handle, bool isPersistent) native global

; Mofifies whether your adjustment is hidden from other mods
Function SetAdjustmentHidden(ObjectReference akRef, int handle, bool isHidden) native global


; Removes all adjustments from all references handled by your mod
Function RemoveAllAdjustments(string espName) native global


; Gets an array of all adjustable node names
string[] Function GetAllNodeNames(ObjectReference akRef) native global


; Gets the node transform values
Transform Function GetNodeTransform(ObjectReference akRef, string nodeName, int handle) native global

; Sets the node transform values
Function SetNodeTransform(ObjectReference akRef, string nodeName, int handle, Transform transform) native global

; This is designed to be used on override nodes
; It will negate the base transform and override it with your own transform
Function OverrideNodeTransform(ObjectReference akRef, string nodeName, int handle, Transform transform) native global

; Resets the node transform back to default values
Function ResetNodeTransform(ObjectReference akRef, string nodeName, int handle) native global


; Gets an array of all node transforms
Transform[] Function GetAllNodeTransforms(ObjectReference akRef, int handle) native global

; Sets the values of all node transforms specified in the array
Function SetAllNodeTransforms(ObjectReference akRef, int handle, Transform[] transforms) native global

; Performs an override on the override transforms and a regular set on offset transforms
Function OverrideAllNodeTransforms(ObjectReference akRef, int handle, Transform[] transforms) native global

; Resets all transforms managed by this handle to their default
Function ResetAllNodeTransforms(ObjectReference akRef, int handle) native global


; Adjustments are stored in Data/F4SE/Plugins/SAF/Adjustments and don't require path/extension for the filename

; Loads a new adjustment and returns the handle
int Function LoadAdjustment(ObjectReference akRef, string filename, string espName, bool persistent, bool hidden) native global

; Saves a new adjustment specified by the handle
Function SaveAdjustment(ObjectReference akRef, string filename, int handle) native global


; Json poses are stored in Data/F4SE/Plugins/SAF/Poses and don't require path/extension for the filename

; Loads a json pose, won't affect persistent adjustments
Function LoadPose(ObjectReference akRef, string filename) native global

; Saves a json pose, won't be affected by persistent adjustments
Function SavePose(ObjectReference akRef, string filename) native global

; Resets a json pose, won't reset persistent adjustments
Function ResetPose(ObjectReference akRef) native global