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

    // ==========================================
    // ЛОГИКА ГОЛОСА ДЛЯ ЛОББИ
    // ==========================================
    
    SCR_VoNComponent GetLobbyVoN()
    {
        Print("[LobbyVoN] GetLobbyVoN called!", LogLevel.DEBUG);
        
        if (!m_LobbyVoN)
        {
            IEntity entity = GetControlledEntity();
            if (entity)
            {
                m_LobbyVoN = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));
                if (m_LobbyVoN)
                    Print("[LobbyVoN] SUCCESS: SCR_VoNComponent found on controlled entity.", LogLevel.DEBUG);
                else
                    Print("[LobbyVoN] ERROR: SCR_VoNComponent NOT found on controlled entity! Check SpectatorPrefab.", LogLevel.ERROR);
            }
            else
            {
                Print("[LobbyVoN] ERROR: Controlled entity is NULL! Player has no dummy.", LogLevel.ERROR);
            }
        }
        return m_LobbyVoN;
    }
 void LobbyVoNEnable()
    {
        Print("[LobbyVoN] >>> LobbyVoNEnable called! >>>", LogLevel.WARNING);
        SCR_VoNComponent von = GetLobbyVoN();
        
        if (von)
        {
            // Используем ТОЛЬКО прямой голос, точно как у коллеги из PS
            von.SetCommMethod(ECommMethod.DIRECT);
            von.SetCapture(true);
            
            Print("[LobbyVoN] SUCCESS: Microphone Capture ON", LogLevel.WARNING);
        }
        else
        {
            Print("[LobbyVoN] ERROR: SCR_VoNComponent NOT found on controlled entity!", LogLevel.ERROR);
        }
    }

    void LobbyVoNDisable()
    {
        Print("[LobbyVoN] >>> LobbyVoNDisable called! >>>", LogLevel.WARNING);
        SCR_VoNComponent von = GetLobbyVoN();
        if (von)
        {
            von.SetCapture(false);
        }
    }
}