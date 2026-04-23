[ComponentEditorProps(category: "Lobby", description: "Custom VoN for Lobby Dummies")]
class LobbyVoNComponentClass : VoNComponentClass {}
class LobbyVoNComponent : VoNComponent 
{
    const int DIRECT_VON_FREQUENCY = 32000; 
    
    protected static ref map<int, float> s_mSpeechTimeouts = new map<int, float>();
    const float SPEECH_TIMEOUT_MS = 350.0;

    override protected event void OnCapture(BaseTransceiver transmitter)
    {
        int localPlayerId = GetGame().GetPlayerController().GetPlayerId();
        UpdateSpeechTime(localPlayerId, transmitter);
    }

    override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
    {        
        if (frequency == DIRECT_VON_FREQUENCY)
        {
            UpdateSpeechTime(playerId, receiver);
        }
    }
    
    protected void UpdateSpeechTime(int playerId, BaseTransceiver transceiver)
    {
        float worldTime = GetGame().GetWorld().GetWorldTime();
        s_mSpeechTimeouts.Set(playerId, worldTime + SPEECH_TIMEOUT_MS);
    }
    
    static bool IsPlayerTalking(int playerId)
    {
        float worldTime = GetGame().GetWorld().GetWorldTime();
        if (!s_mSpeechTimeouts.Contains(playerId)) return false;
        return s_mSpeechTimeouts.Get(playerId) >= worldTime;
    }
    
    static void RemovePlayer(int playerId)
    {
        s_mSpeechTimeouts.Remove(playerId);
    }
}