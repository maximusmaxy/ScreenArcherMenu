Scriptname ScreenArcherMenu extends ReferenceAlias

InputEnableLayer inputLayer = None

Actor Property player Auto

event OnInit()
    RegisterMenu()
endevent

event OnPlayerLoadGame()
    RegisterMenu()
endevent

event OnMenuOpenCloseEvent(string asMenuName, bool abOpening) 
    if (abOpening)
        if (inputLayer == None)
            inputLayer = InputEnableLayer.Create()
        endif 

        ;Movement, fighting, camswitch, looking, sneaking, menu, activate, journal, vats, favorites, running
        inputLayer.DisablePlayerControls(true, true, true, true, true, true, true, true, true, true, true)
    else
        if (inputLayer != None)
            inputLayer.Reset()
            inputLayer = None
        endif
    endif
endevent

function SetMovement(bool move)
    if (inputLayer != None)
        inputLayer.EnableLooking(move)
        inputLayer.EnableMovement(move)
    endif
endfunction

function RegisterMenu()
    if (!UI.IsMenuRegistered("ScreenArcherMenu"))
        UI:MenuData mData = new UI:MenuData
        ;2          Always Open
        ;4          Use Cursor
        ;10         Modal
        ;80         Disables pause menu
        ;400        Update uses cursor
        ;8000       Custom Rendering
        ;10000      AssignCursorToRenderer
        ;8000000    UsesMovementToDirection

        mData.menuFlags = 0x8018496
        mData.extendedFlags = 0x3
        UI.RegisterCustomMenu("ScreenArcherMenu", "ScreenArcherMenu", "root1.Menu_mc", mData)
    endif

    RegisterForMenuOpenCloseEvent("ScreenArcherMenu")
endfunction

function ToggleMenu() global
    SAM.ToggleMenu()
endfunction

function ForceRegister() global
    Quest samQuest = Game.GetFormFromFile(0x800, "ScreenArcherMenu.esp") as Quest
    ScreenArcherMenu playerRef = samQuest.GetAlias(0) as ScreenArcherMenu
    playerRef.RegisterMenu()
endfunction

function StartQuest() global
    Quest samQuest = Game.GetFormFromFile(0x800, "ScreenArcherMenu.esp") as Quest
    samQuest.Reset()
    ReferenceAlias playerRef = samQuest.GetAlias(0) as ReferenceAlias
    playerRef.ForceRefTo(Game.GetPlayer())
    samQuest.Start()
endfunction