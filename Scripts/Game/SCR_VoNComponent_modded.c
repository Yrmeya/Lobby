modded class SCR_VoNComponent
{
    static ref map<int, float> s_LobbySpeechEndTime = new map<int, float>();
    static ref map<int, bool>  s_LobbySpeechIsChannel = new map<int, bool>();

    override protected event void OnCapture(BaseTransceiver transmitter)
    {
        super.OnCapture(transmitter);

        PlayerController pc = GetGame().GetPlayerController();
        if (!pc) return;

        int localId = pc.GetPlayerId();
        s_LobbySpeechEndTime.Set(localId, GetGame().GetWorld().GetWorldTime() + 100);
    }

    override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
    {
        super.OnReceive(playerId, isSenderEditor, receiver, frequency, quality);

        s_LobbySpeechEndTime.Set(playerId, GetGame().GetWorld().GetWorldTime() + 100);
        s_LobbySpeechIsChannel.Set(playerId, frequency == 32000);
    }

    static bool LobbyIsPlayerTalking(int playerId)
    {
        float endTime;
        if (s_LobbySpeechEndTime.Find(playerId, endTime))
            return endTime > GetGame().GetWorld().GetWorldTime();

        return false;
    }
};