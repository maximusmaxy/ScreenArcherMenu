Scriptname SAM Native Hidden

;---------------------------------------------------------------
; Menu Actions
;---------------------------------------------------------------

; If SAM is closed, this will directly open the specified menu
; If SAM is already open this function will be ignored
; If called while the specified menu is already open, it will be closed instead
; Most SAM menus are not designed to be opened this way, it is primarily intended to open extensions directly
Function OpenMenu(string menuName) native global

; Pushes a new menu to the top of the stack. Won't do anything if the menu is closed
Function PushMenu(string menuName) native global

; Pops the top menu from the stack
Function PopMenu() native global

; Pops the top menu from the stack until menuName is reached. Does nothing if menuName isn't on the stack.
Function PopMenuTo(string menuName) native global

; Displays a notification in the top left of the menu. This is not queued and will overwrite any existing notificaton. 
; Use SAM.SetNotification instead for callbacks
Function ShowNotification(string msg) native global

;---------------------------------------------------------------
; Papyrus Callbacks
;---------------------------------------------------------------

; To send menu values to sam from a get function you will need to send an array of values to set the items to
; List can be int, float, bool, string or null and will return the value specified when selected
; Checkboxes must be boolean values
; Sliders must be int or float depending on the type specified in the slider info
; Touch sliders must be float values
Function SetValues(var[] values) native global

; Same as above but dynamically assign menu item names as well
Function SetItems(string[] names, var[] values) native global

; For List get functions, Sets the both the names and values
Function SetNames(string[] names) native global

; This is for notification callbacks
Function SetNotification(string msg) native global

; This is for title callbacks
Function SetTitle(string msg) native global
 
; You can request set functions to require a callback using the { "wait": true } property and return a success with this function
; This allows you to optionally provide error notifications on set functions
Function SetSuccess() native global

; You can return an error using this function
; This prevents get functions from loading the menu
; You can also optionally use this for set functions using the { "wait": true } property
Function SetError(string error) native global

;---------------------------------------------------------------
; References
;---------------------------------------------------------------

; Gets a reference to the SAM target, limited to actors only
ObjectReference Function GetRefr() native global

; Gets a reference to the SAM target, non actors are allowed
ObjectReference Function GetNonActorRefr() native global

;---------------------------------------------------------------
; Util
;---------------------------------------------------------------

; Gets the filename from a path without extension
string Function GetFilename(string path) native global

; Removes the root and the extension from a path leaving the relative path
string Function GetRelativePath(string root, string extension, string path) native global

;---------------------------------------------------------------
; Debug
;---------------------------------------------------------------

; Writes the parsed json menu to the sam.log
function LogMenu(string menuName) native global

; Reloads the menu data
function ReloadMenus() native global

; Closes the menu and clears persistent data to prevent getting locked
function ForceQuit() native global

;---------------------------------------------------------------
; Internal
;---------------------------------------------------------------

; Used by MCM for the menu toggle hotkey
Function ToggleMenu() native global

; Reregister for debugging purposes
Function ReregisterMenu() native global