[ComponentEditorProps(category: "Lobby", description: "Lobby RPC Component")]
class LobbyRPCComponentClass : ScriptComponentClass {}

class LobbyRPCComponent : ScriptComponent
{
    static const int ROLE_DEBOUNCE_MS = 300;
    protected float m_fLastRoleRequest = 0;

    void BroadcastHeaderUpdate()
    {
        if (!Replication.IsServer()) return;
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (!mgr) return;
        Rpc(RPC_SV_HeaderUpdate, mgr.GetPlayerCount(), mgr.GetReadyCount());
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_SV_HeaderUpdate(int playerCount, int readyCount)
    {
        LobbyMenu ui = LobbyMenu.GetInstance();
        if (ui) ui.OnHeaderUpdated(playerCount, readyCount);
    }

    void BroadcastPlayerJoined(int playerId, string playerName)
    {
        if (!Replication.IsServer()) return;
        Rpc(RPC_SV_PlayerJoined, playerId, playerName);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_SV_PlayerJoined(int playerId, string playerName)
    {
        LobbyMenu ui = LobbyMenu.GetInstance();
        if (ui) ui.OnPlayerJoined(playerId, playerName);
    }

    void BroadcastPlayerLeft(int playerId)
    {
        if (!Replication.IsServer()) return;
        Rpc(RPC_SV_PlayerLeft, playerId);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_SV_PlayerLeft(int playerId)
    {
        LobbyMenu ui = LobbyMenu.GetInstance();
        if (ui) ui.OnPlayerLeft(playerId);
    }

    void BroadcastPlayerAssignment(int playerId, int squadIndex, int roleIndex)
    {
        if (!Replication.IsServer()) return;
        Rpc(RPC_SV_PlayerAssign, playerId, squadIndex, roleIndex);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_SV_PlayerAssign(int playerId, int squadIndex, int roleIndex)
    {
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (mgr)
        {
            LobbyPlayerData data = mgr.GetPlayerData(playerId);
            if (data)
            {
                data.m_iSquadIndex = squadIndex;
                data.m_iRoleIndex = roleIndex;
            }
        }
        LobbyMenu ui = LobbyMenu.GetInstance();
        if (ui) ui.OnPlayerAssigned(playerId, squadIndex, roleIndex);
    }

    void BroadcastLobbyClose()
    {
        if (!Replication.IsServer()) return;
        Rpc(RPC_SV_LobbyClose);
        GetGame().GetCallqueue().CallLater(HandleLobbyClose, 300, false);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_SV_LobbyClose()
    {
        GetGame().GetCallqueue().CallLater(HandleLobbyClose, 300, false);
    }

    protected void HandleLobbyClose()
    {
        LobbyMenu ui = LobbyMenu.GetInstance();
        if (ui) GetGame().GetMenuManager().CloseMenu(ui);
        GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.WelcomeScreenMenu);
    }

    void BroadcastJIPSpectator(int playerId)
    {
        if (!Replication.IsServer()) return;
        Rpc(RPC_SV_JIPSpectator, playerId);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_SV_JIPSpectator(int playerId)
    {
        PlayerController pc = GetGame().GetPlayerController();
        if (pc && pc.GetPlayerId() == playerId)
        {
            LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
            if (mgr) mgr.SetLocalJIPSpectator(true);
        }
    }

    void BroadcastJIPCameraSpawn(int playerId, vector spawnPos)
    {
        if (!Replication.IsServer()) return;
        Rpc(RPC_SV_JIPCameraSpawn, playerId, spawnPos);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_SV_JIPCameraSpawn(int playerId, vector spawnPos)
    {
        PlayerController pc = GetGame().GetPlayerController();
        if (pc && pc.GetPlayerId() == playerId)
        {
            GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.WelcomeScreenMenu);
            GetGame().GetCallqueue().CallLater(SpawnGhostSafe, 50, false, playerId, spawnPos);
        }
    }

        void SpawnGhostSafe(int playerId, vector spawnPos)
    {
        // Закрываем вообще ВСЕ меню, включая ванильный EditorMenu, который открывается при смерти
        GetGame().GetMenuManager().CloseAllMenus();
        
        // Дополнительно принудительно закрываем менеджер редактора
        SCR_EditorManagerCore editorCore = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
        if (editorCore)
        {
            SCR_EditorManagerEntity editorManager = editorCore.GetEditorManager();
            if (editorManager && editorManager.IsOpened()) 
                editorManager.Close();
        }
        
        PlayerDeathGhost.SpawnGhostStatic(playerId, spawnPos);
    }

    void SV_FullSync(int playerId)
    {
        if (!Replication.IsServer()) return;
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (!mgr) return;
        foreach (int pid, LobbyPlayerData data : mgr.GetPlayersMap())
        {
            BroadcastPlayerJoined(pid, data.m_sPlayerName);
            BroadcastPlayerAssignment(pid, data.m_iSquadIndex, data.m_iRoleIndex);
        }
        BroadcastHeaderUpdate();
    }

    void SV_SelectRole(int playerId, int squadIndex, int roleIndex)
    {
        if (!Replication.IsServer()) return;
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (mgr) mgr.AssignRole_S(playerId, squadIndex, roleIndex);
    }

    void SV_StartGame()
    {
        if (!Replication.IsServer()) return;
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (mgr) mgr.RequestStartGame();
    }

    void RequestSelectRole(int squadIndex, int roleIndex)
    {
        float now = GetGame().GetWorld().GetWorldTime();
        if (now - m_fLastRoleRequest < ROLE_DEBOUNCE_MS) return;
        m_fLastRoleRequest = now;
        SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
        if (pc) pc.RequestLobbySelectRole(squadIndex, roleIndex);
    }

    void RequestStartGame()
    {
        SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
        if (pc) pc.RequestLobbyStartGame();
    }

    void RequestFullSync()
    {
        SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
        if (pc) pc.RequestLobbyFullSync();
    }

    static LobbyRPCComponent GetInstance()
    {
        SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (!gm) return null;
        return LobbyRPCComponent.Cast(gm.FindComponent(LobbyRPCComponent));
    }
}