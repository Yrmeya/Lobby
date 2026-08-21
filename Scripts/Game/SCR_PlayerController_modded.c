modded class SCR_PlayerController
{
    protected SCR_VoNComponent m_LobbyVoN;

    void RequestLobbyFullSync()
    {
        Rpc(RPC_Lobby_FullSync, GetPlayerId());
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Server)]
    protected void RPC_Lobby_FullSync(int playerId)
    {
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc) rpc.SV_FullSync(playerId);
    }

    void RequestLobbySelectRole(int squadIndex, int roleIndex)
    {
        Rpc(RPC_Lobby_SelectRole, squadIndex, roleIndex);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Server)]
    protected void RPC_Lobby_SelectRole(int squadIndex, int roleIndex)
    {
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc) rpc.SV_SelectRole(GetPlayerId(), squadIndex, roleIndex);
    }

    void RequestLobbyStartGame()
    {
        Rpc(RPC_Lobby_StartGame);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Server)]
    protected void RPC_Lobby_StartGame()
    {
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc) rpc.SV_StartGame();
    }

    void RequestLobbyMapPoint(int worldX10, int worldY10)
    {
        Rpc(RPC_Lobby_MapPoint, worldX10, worldY10);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Server)]
    protected void RPC_Lobby_MapPoint(int worldX10, int worldY10)
    {
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc) rpc.SV_PlaceMapPoint(GetPlayerId(), worldX10, worldY10);
    }

    void RequestLobbySpawnCiv()
    {
        Rpc(RPC_Lobby_SpawnCiv);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Server)]
    protected void RPC_Lobby_SpawnCiv()
    {
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc) rpc.SV_SpawnCiv(GetPlayerId());
    }

    // ==========================================
    // ЛОГИКА ГОЛОСА ДЛЯ ЛОББИ
    // ==========================================
    
    SCR_VoNComponent GetLobbyVoN()
    {
        if (!m_LobbyVoN)
        {
            IEntity entity = GetControlledEntity();
            if (entity)
                m_LobbyVoN = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));
        }
        return m_LobbyVoN;
    }

    void LobbyVoNEnable()
    {
        SCR_VoNComponent von = GetLobbyVoN();
        if (von)
        {
            // Используем чистый прямой голос. 
            // Так как мы спавнимся в одной точке, дистанция не проблема.
            von.SetCommMethod(ECommMethod.DIRECT);
            von.SetCapture(true);
        }
    }

    void LobbyVoNDisable()
    {
        SCR_VoNComponent von = GetLobbyVoN();
        if (von)
        {
            von.SetCapture(false);
        }
    }
}