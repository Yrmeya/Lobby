modded class SCR_PlayerController
{
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
}