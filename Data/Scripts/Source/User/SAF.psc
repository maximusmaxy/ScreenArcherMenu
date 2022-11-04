Scriptname SAF Native Hidden

Struct Transform
    string name
    bool offset
    float x
    float y
    float z
    float yaw
    float pitch
    float roll
    float scale
EndStruct

; This creates a new adjustment on the reference. 
; The integer returned by this function is used as a handle to interact with this specific adjustment
; You have to specificy your mod's .esp/.esl filename so it can erase the adjustment if the mod isn't found
; Persistent saves the adjustment to the co-save
; Hidden hides your adjustment from other mods
; eg. CreateAdjustment(ref, "Sam Adjustment", "ScreenArcherMenu.esp", false, true)
int Function CreateAdjustment(ObjectReference akRef, string name, string espName) native global

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

; Removes all adjustments from all references handled by your mod
Function RemoveModAdjustments(string espName) native global

; Removes all adjustments from all actors, clears the persistence store and race adjustments
; This is intended to be a failsafe for end users and not intended for use by modders
; To call in game use the console command - cgf "saf.RemoveAllAdjustments" -
Function RemoveAllAdjustments() native global


; Gets an array of all adjustable node names
string[] Function GetAllNodeNames(ObjectReference akRef) native global

; Gets an adjustment by filename and returns the handle
int Function GetAdjustmentFile(ObjectReference akRef, string filename) native global

; Removes the adjustment by filename
Function RemoveAdjustmentFile(ObjectReference akRef, string filename) native global


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
int Function LoadAdjustment(ObjectReference akRef, string filename, string espName) native global

; Saves a new adjustment specified by the handle
Function SaveAdjustment(ObjectReference akRef, string filename, int handle) native global


; Json poses are stored in Data/F4SE/Plugins/SAF/Poses and don't require path/extension for the filename

; Loads a json pose, won't affect persistent adjustments
Function LoadPose(ObjectReference akRef, string filename) native global

; Saves a json pose, won't be affected by persistent adjustments
Function SavePose(ObjectReference akRef, string filename) native global

; Resets a json pose, won't reset persistent adjustments
Function ResetPose(ObjectReference akRef) native global


; Eye Control
; You will need to use the eye tracking hack to disable eye tracking or the eye tracking will overwrite your coords

; Gets the x/y coordinates of the eyes
float[] Function GetEyeCoords(ObjectReference akRef) native global

; Sets the x/y coordinates of the eyes
Function SetEyeCoords(ObjectReference akRef, float x, float y) native global


; Hacks
; Blink hack disables blinking, allowing access to the eyelid down morphs 18 and 41
; Eye tracking hack disables eye tracking, allowing acces to eye coords functions without being overwritten
; Morphs hack forces the update of morphs, preventing the morph flickering bug

; Gets the state of the blink hack
bool Function GetBlinkHack() native global

; Set true to disable blinking, set false to reenable blinking
Function SetBlinkHack(bool enabled) native global

; Gets the state of the eye tracking hack
bool Function GetEyeTrackingHack() native global

; Set true to disable eye tracking, set false to reenable eye tracking
Function SetEyeTrackingHack(bool enabled) native global

; Gets the state of the morphs hack
bool Function GetMorphsHack() native global

; Set true to prevent morphs flickering
Function SetMorphsHack(bool enabled) native global