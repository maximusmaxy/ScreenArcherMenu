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

;---------------------------------------------------------------
; Adjustments
;---------------------------------------------------------------

; This creates a new adjustment on the reference. 
; The integer returned by this function is used as a handle to interact with this specific adjustment
; You have to specificy your mod's .esp/.esl filename so it can erase the adjustment if the mod isn't found
; Persistent saves the adjustment to the co-save
; Hidden hides your adjustment from other mods
; eg. CreateAdjustment(ref, "Sam Adjustment", "ScreenArcherMenu.esp")
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

; Gets all adjustment handles with the specified name.
; Names are not unique and can be edited by the player so it is recommended to avoid using this
int[] Function GetAdjustmentsByName(ObjectReference akRef, string name) native global

; Gets all adjustment handles for a specific type. The suported types are:
; Default - This is the default "New Adjustment" type
; Skeleton - This is the type used after an adjustment is saved or loaded from the pose/skeleton adjustments menu
; Race - This is the type used when loaded by the race adjustments menu
int[] Function GetAdjustmentsByType(ObjectReference akRef, string type) native global
 
; Gets the uniquely typed adjustment. The supported types are:
; Pose - Used when loading a SAM pose
; Tongue - Generated when loading a face morph that includes a tongue adjustment
int Function GetAdjustmentByUniqueType(ObjectReference akRef, string type) native global

; Removes all adjustments from all references handled by your mod
Function RemoveModAdjustments(string espName) native global

; Removes all adjustments from all actors, clears the persistence store and race adjustments
; This is intended to be a failsafe for end users and not intended for use by modders
; To call in game use the console command - cgf "saf.RemoveAllAdjustments" -
Function RemoveAllAdjustments() native global

; Gets an adjustment by filename and returns the handle
int Function GetAdjustmentFile(ObjectReference akRef, string filename) native global

; Removes the adjustment by filename
Function RemoveAdjustmentFile(ObjectReference akRef, string filename) native global


;---------------------------------------------------------------
; Node Lists
;---------------------------------------------------------------

; Gets an array of all adjustable node names
string[] Function GetAllNodeNames(ObjectReference akRef) native global

; Gets an array of all node names not including the offsets
string[] Function GetBaseNodeNames(ObjectReference akRef) native global

; Gets an array of the center node names
string[] Function GetCenterNodeNames(ObjectReference akRef) native global

; Gets an array of the left node names, use in parallel with GetRightNodeNames
string[] Function GetLeftNodeNames(ObjectReference akRef) native global

; Gets an array of the right node names, use in parallel with GetLeftNodeNames
string[] Function GetRightNodeNames(ObjectReference akRef) native global


;---------------------------------------------------------------
; Transforms
;---------------------------------------------------------------

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


;---------------------------------------------------------------
; Adjustment Scale
;---------------------------------------------------------------

; Gets the scale value of the adjustment
int Function GetAdjustmentScale(ObjectReference akRef, int handle) native global

; Sets the scale value of the adjustment
Function SetAdjustmentScale(ObjectReference akRef, int handle, int scale) native global

; Gets the node transform modified by the scale value of the adjustment
Transform Function GetNodeTransformScaled(ObjectReference akRef, string nodeName, int handle) native global


;---------------------------------------------------------------
; Adjustment Files
;---------------------------------------------------------------

; Adjustments are stored in Data/F4SE/Plugins/SAF/Adjustments and don't require path/extension for the filename

; Loads a new adjustment and returns the handle
int Function LoadAdjustment(ObjectReference akRef, string filename, string espName) native global

; Saves a new adjustment specified by the handle
Function SaveAdjustment(ObjectReference akRef, string filename, int handle) native global

; Loads an adjustment to the cache. 
; This does not load the adjustment on the reference however it still needs a reference for backwards compatibility purposes.
Function CacheAdjustment(ObjectReference akRef, string filename) native global

; Loads an adjustment from the cache and returns the handle
int Function LoadCachedAdjustment(ObjectReference akRef, string filename, string espName) native global


;---------------------------------------------------------------
; Pose Files
;---------------------------------------------------------------

; Json poses are stored in Data/F4SE/Plugins/SAF/Poses and don't require path/extension for the filename

; Loads a json pose, won't affect persistent adjustments
Function LoadPose(ObjectReference akRef, string filename) native global

; Saves a json pose, won't be affected by persistent adjustments
Function SavePose(ObjectReference akRef, string filename) native global

; Resets a json pose, won't reset persistent adjustments
Function ResetPose(ObjectReference akRef) native global


;---------------------------------------------------------------
; Eyes
;---------------------------------------------------------------

; You will need to use the eye tracking hack to disable eye tracking or the eye tracking will overwrite your coords

; Gets the x/y coordinates of the eyes
float[] Function GetEyeCoords(ObjectReference akRef) native global

; Sets the x/y coordinates of the eyes
Function SetEyeCoords(ObjectReference akRef, float x, float y) native global


;---------------------------------------------------------------
; Hacks
;---------------------------------------------------------------

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


; ;---------------------------------------------------------------
; ; Transform Handles
; ;---------------------------------------------------------------

; ; To increase the performance and functionality of transform operations from papyrus you can perform your operations through a handle
; ; The transforms are stored and operated on natively
; ; Handles are unique for each load and are not persistent
; ; To prevent memory leaks you are required to delete your transform handles after use

; ; Creates a new identity transform and returns the handle
; int Function CreateTransformHandle() native global

; ; Creates a new transform handle from a local transform struct and returns the handle
; int Function CreateTransformHandleLocal(Transform transform) native global

; ; Sets the values of transform handle A to the values of transform handle B
; Function SetTransformHandle(int aHandle, int bHandle) native global

; ; Makes a copy of an existing transform and returns the new handle
; int Function CopyTransformHandle(int tHandle) native global

; ; Deletes the transform
; Function DeleteTransformHandle(int tHandle) native global

; ; To prevent memory leaks from early returns, you can instead register your handles to your mod and delete them later
; Function RegisterTransformHandle(string espName, int tHandle) native global

; ; Deletes all handles registered to your mod
; Function DeleteRegisteredTransformHandles(string espName) native global

; ;---------------------------------------------------------------
; ; Transform Handle Get
; ;---------------------------------------------------------------

; ; Gets a transform handle from an actors adjustment, 0 on failure
; int Function GetAdjustmentTransformHandle(ObjectReference akRef, int aHandle, string nodeName, bool offset) native global

; ; Sets a transform handle to an actors adjustment
; Function SetAdjustmentTransformHandle(ObjectReference akRef, int aHandle, string nodeName, bool offset, int tHandle) native global

; ; Gets a transform handle from the actors current skeleton position, 0 on failure
; int Function GetSkeletonTransformHandle(ObjectReference akRef, string nodeName) native global

; ; Gets a transform handle from the actors default a-pose skeleton position, 0 on failure
; int Function GetBaseTransformHandle(ObjectReference akRef, string nodeName) native global

; ; Copies an actors adjustment to the specified handle, this allows you to reuse handles, false on failure
; bool Function CopyAdjustmentTransformHandle(ObjectReference akRef, int aHandle, string nodeName, bool offset, int tHandle) native global

; ; Copies the actors current skeleton position to the specified handle, false on failure
; bool Function CopySkeletonTransformHandle(ObjectReference akRef, string nodeName, int tHandle) native global

; ; Copies the actors default a-pose position to the specified handle, false on failure
; bool Function CopyBaseTransformHandle(ObjectReference akRef, string nodeName, int tHandle) native global

; ; Gets the local transform struct representation of the transform handle, Avoid using this for operations
; Transform Function GetTransformLocal(int tHandle) native global

; ; Sets the local transform struct to the transform handle, Avoid using this for operations
; Function SetTransformLocal(int tHandle, Transform transform) native global


; ;---------------------------------------------------------------
; ; Transform Operations
; ;---------------------------------------------------------------

; ; I could not find an easy online resource to explain 3D transformation mathematics. If you know of one let me know. I will try to explain it the best i can
; ; Unlike regular mathematics, combining transforms is directional and not additive. Assuming A and B are transforms
; ; 
; ; A -> B is not equivalent to B -> A
; ;
; ; The order in which they are applied doesn't matter, only the direction
; ; if X = B -> C
; ; A -> B -> C is equivalent to A -> X
; ; 
; ; You can go backwards by applying the inverse of a transform
; ; A is equivalent to A -> B -> Inverse(B)
; ;
; ; The Identity transform is the root position with no translation/rotation and the scale is 1.0f

; ; Gets the result of A -> B
; int Function GetAppliedTransform(int aHandle, int bHandle) native global

; ; Applies the result of A -> B directly to A
; Function ApplyTransform(int aHandle, int bHandle) native global

; ; Gets the transform such that A -> Inverse = Identity
; int Function GetInverseTransform(int aHandle) native global

; ; Applies the inverse directly to A
; Function InverseTransform(int aHandle) native global

; ; Gets the transform such that A -> Difference = B
; int Function GetDifferenceTransform(int aHandle, int bHandle) native global

; ; Applies the difference directly to A
; Function DifferenceTransform(int aHandle, int bHandle) native global

; ; Gets the transform between Identity -> A such that 0.0f is the identity and 1.0f is A
; ; Translation and scale are calculated using multiplication and rotation is calculated using spherical linear interpolation
; int Function GetBetweenTransform(int aHandle, float scalar) native global

; ; Applies the between transform directly to A
; Function BetweenTransform(int aHandle, float scalar) native global

; ; Gets the transform position as a float array [x, y, z]
; float[] Function GetTransformPosition(int aHandle) native global

; ; Sets the transform position
; Function SetTransformPosition(int aHandle, float[] pos) native global

; ; Gets an array of values that represent a 3x3 rotation matrix. Array index refers to the following matrix rows and columns
; ; 0, 1, 2
; ; 3, 4, 5
; ; 6, 7, 8
; float[] Function GetTransformRotation(int aHandle) native global

; ; Sets the transforms rotation matrix
; Function SetTransformRotation(int aHandle, float[] matrix) native global

; ; Rotates the 3x3 rotation matrix around an axis by the scalar in radians
; ; Axis constants: X = 1, Y = 2, Z = 3
; Function RotateTransformAxis(int aHandle, int axis, float scalar) native global

; ; Gets the transforms scale
; float Function GetTransformScale(int aHandle) native global

; ; Sets the transforms scale
; Function SetTransformScale(int aHandle, float scale) native global