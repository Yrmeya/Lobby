modded class SCR_PlayerDeployMenuHandlerComponent
{
    override bool CanOpenMenu()
    {
        PlayerController pc = GetGame().GetPlayerController();
        
        if (pc && PlayerDeathGhost.IsPlayerDead(pc.GetPlayerId()))
            return false;

        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (mgr)
        {
            if (mgr.IsLobbyActive())
                return false;

            if (mgr.IsLocalPlayerJIPSpectator())
                return false;
        }

        return super.CanOpenMenu();
    }
}

modded class SCR_WelcomeScreenMenu
{
    override void OnMenuOpen()
    {
        super.OnMenuOpen();

        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (!mgr) return;

        if (mgr.IsLobbyActive())
        {
            Widget root = GetRootWidget();
            if (root) root.SetOpacity(0);
            GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.LobbyMenu);
            return;
        }

        if (mgr.IsLocalPlayerJIPSpectator())
        {
            Widget root = GetRootWidget();
            if (root) root.SetOpacity(0);
            return;
        }
    }
}

modded class SCR_DeployMenuMain
{
    override void OnMenuOpen()
    {
        super.OnMenuOpen();

        PlayerController pc = GetGame().GetPlayerController();
        
        if (pc && PlayerDeathGhost.IsPlayerDead(pc.GetPlayerId()))
        {
            Close();
            return;
        }

        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (!mgr) return;

        if (mgr.IsLobbyActive() || mgr.IsLocalPlayerJIPSpectator())
        {
            Close();
            return;
        }
    }
}