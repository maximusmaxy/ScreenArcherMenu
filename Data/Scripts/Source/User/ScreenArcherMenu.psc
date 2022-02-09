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

        Var[] args = new Var[1];
        args[0] = self;
        UI.Invoke("ScreenArcherMenu", "root1.Menu_mc.setScriptHandle", args)
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
        mData.menuFlags = 0x8018494
        mData.extendedFlags = 0x3
        UI.RegisterCustomMenu("ScreenArcherMenu", "ScreenArcherMenu", "root1.Menu_mc", mData)
    endif

    RegisterForMenuOpenCloseEvent("ScreenArcherMenu")
endfunction

function ToggleMenu() global
    if (UI.IsMenuOpen("ScreenArcherMenu"))
        UI.Invoke("ScreenArcherMenu", "root1.Menu_mc.tryClose")
    else
        UI.OpenMenu("ScreenArcherMenu")
    endif
endfunction