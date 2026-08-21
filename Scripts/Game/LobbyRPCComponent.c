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

    // ВАЖНО: RPC-параметры типа float не реплицируются в этом движке -
    // передаём координаты целыми с точностью 0.1 метра.
    void RequestPlaceMapPoint(float worldX, float worldY)
    {
        SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
        if (pc) pc.RequestLobbyMapPoint((int)(worldX * 10.0), (int)(worldY * 10.0));
    }

    void SV_PlaceMapPoint(int playerId, int worldX10, int worldY10)
    {
        if (!Replication.IsServer()) return;
        Print("[LobbyMap] SV_PlaceMapPoint pid=" + playerId + " w10=(" + worldX10 + "," + worldY10 + ")");
        Rpc(RPC_SV_MapPoint, playerId, worldX10, worldY10);

        // В оффлайн-сессии (Workbench) broadcast RPC не доставляется локальной
        // сессии - обновляем UI напрямую. Определяем по числу игроков.
        PlayerManager pm = GetGame().GetPlayerManager();
        ref array<int> players = {};
        if (pm) pm.GetPlayers(players);
        if (players.Count() <= 1)
        {
            LobbyMenu ui = LobbyMenu.GetInstance();
            if (ui) ui.OnMapPointReceived(playerId, worldX10 * 0.1, worldY10 * 0.1);
        }
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_SV_MapPoint(int playerId, int worldX10, int worldY10)
    {
        Print("[LobbyMap] RPC_SV_MapPoint received pid=" + playerId);
        LobbyMenu ui = LobbyMenu.GetInstance();
        if (ui) ui.OnMapPointReceived(playerId, worldX10 * 0.1, worldY10 * 0.1);
    }

    //------------------------------------------------------------------------------------------------
    //! #spawnciv: после вселения в нового солдата возвращаем клиенту камеру персонажа
    //! и удаляем призрачную камеру смерти (ManualCamera), которая осталась активной.
    void BroadcastRespawnPlayer(int playerId)
    {
        if (!Replication.IsServer()) return;
        Rpc(RPC_SV_RespawnPlayer, playerId);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RPC_SV_RespawnPlayer(int playerId)
    {
        PlayerController pc = GetGame().GetPlayerController();
        if (!pc || pc.GetPlayerId() != playerId) return;

        // Ждём, пока клиент получит нового подопечного, и возвращаем камеру персонажу
        GetGame().GetCallqueue().CallLater(RestoreCharacterCameraOnClient, 200, false, 0);
    }

    protected void RestoreCharacterCameraOnClient(int attempts)
    {
        if (attempts > 20) return;

        PlayerController pc = GetGame().GetPlayerController();
        if (!pc) return;

        int playerId = pc.GetPlayerId();
        IEntity ghostCam = PlayerDeathGhost.s_PlayerGhostCameras.Get(playerId);

        // Призрачная камера смерти - это SCR_ManualCamera. Она умеет сама
        // вернуть камеру на контролируемого персонажа.
        SCR_ManualCamera manualCam = SCR_ManualCamera.Cast(ghostCam);
        if (manualCam)
        {
            if (manualCam.TrySwitchToControlledEntityCamera())
            {
                delete ghostCam;
                PlayerDeathGhost.s_PlayerGhostCameras.Remove(playerId);
                Print("[LobbyMgr] Camera restored to character camera for player " + playerId);
                return;
            }
        }
        else if (ghostCam)
        {
            // Не manual-камера: возвращаем предыдущую камеру и удаляем призрака
            SCR_CameraManager camMgr = SCR_CameraManager.Cast(GetGame().GetCameraManager());
            if (camMgr) camMgr.SetPreviousCamera();
            delete ghostCam;
            PlayerDeathGhost.s_PlayerGhostCameras.Remove(playerId);
            return;
        }

        // Камера персонажа ещё не готова (репликация possession) - пробуем ещё раз
        GetGame().GetCallqueue().CallLater(RestoreCharacterCameraOnClient, 100, false, attempts + 1);
    }

    //------------------------------------------------------------------------------------------------
    //! #spawnciv: только гейммастер (хост) может возродить погибших.
    void SV_SpawnCiv(int senderId)
    {
        if (!Replication.IsServer()) return;

        PlayerManager pm = GetGame().GetPlayerManager();
        ref array<int> ids = {};
        if (pm) pm.GetPlayers(ids);

        if (ids.IsEmpty() || senderId != ids[0])
        {
            Print("[LobbyMgr] #spawnciv: player " + senderId + " is not the game master.");
            return;
        }

        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (mgr) mgr.SV_SpawnAllDeadPlayers();
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