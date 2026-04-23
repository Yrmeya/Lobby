//------------------------------------------------------------------------------------------------
// Переименованный и адаптированный скрипт для извлечения playerId из VoN движка
//------------------------------------------------------------------------------------------------

void Melis_ScriptInvokerOnCaptureMethod(BaseTransceiver transmitter);
typedef func Melis_ScriptInvokerOnCaptureMethod;
typedef ScriptInvokerBase<Melis_ScriptInvokerOnCaptureMethod> Melis_ScriptInvokerOnCapture;

void Melis_ScriptInvokerOnReceiveMethod(int playerId, BaseTransceiver receiver, int frequency, float quality);
typedef func Melis_ScriptInvokerOnReceiveMethod;
typedef ScriptInvokerBase<Melis_ScriptInvokerOnReceiveMethod> Melis_ScriptInvokerOnReceive;

//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/VoN", description: "Custom VoN Component to properly expose Player IDs")]
class Melis_LobbyVoNComponentClass : VoNComponentClass {}
class Melis_LobbyVoNComponent : VoNComponent 
{
    const float TRANSMISSION_TIMEOUT_MS = 400;
    protected float m_fTransmittingTimeout;
    ref map<int, float> m_fPlayerSpeechReceiveTime = new map<int, float>();
    ref map<int, bool> m_fPlayerSpeechReceiveIsChannel = new map<int, bool>();
    
    ref Melis_ScriptInvokerOnReceive m_ScriptInvokerOnReceiveStart = new Melis_ScriptInvokerOnReceive();
    Melis_ScriptInvokerOnReceive GetOnReceiveStart()
    {
        return m_ScriptInvokerOnReceiveStart;
    }
    
    ref ScriptInvokerInt m_ScriptInvokerOnReceiveEnd = new ScriptInvokerInt();
    ScriptInvokerInt GetOnReceiveEnd()
    {
        return m_ScriptInvokerOnReceiveEnd;
    }
    
    ref Melis_ScriptInvokerOnCapture m_ScriptInvokerOnCaptured = new Melis_ScriptInvokerOnCapture();
    Melis_ScriptInvokerOnCapture GetOnCaptured()
    {
        return m_ScriptInvokerOnCaptured;
    }
    
    float GetPlayerSpeechTime(int playerId)
    {
        if (!m_fPlayerSpeechReceiveTime.Contains(playerId)) return 0.0;
        return m_fPlayerSpeechReceiveTime[playerId];
    }
    
    bool IsPlayerSpeechInChannel(int playerId)
    {		
        PlayerController playerController = GetGame().GetPlayerController();
        if (playerController.GetPlayerId() == playerId) {
            return GetCommMethod() == ECommMethod.SQUAD_RADIO;
        }
        
        if (!m_fPlayerSpeechReceiveIsChannel.Contains(playerId)) return false;
        return m_fPlayerSpeechReceiveIsChannel[playerId];
    }
    
    bool IsPlayerSpeech(int playerId)
    {
        float worldTime = GetGame().GetWorld().GetWorldTime();
        return GetPlayerSpeechTime(playerId) > worldTime;
    }
    
    bool IsTransmitting()
    {
        float worldTime = GetGame().GetWorld().GetWorldTime();
        return m_fTransmittingTimeout >= worldTime;
    }
    
    override protected event void OnCapture(BaseTransceiver transmitter)
    {
        int playerId = GetGame().GetPlayerController().GetPlayerId();
        OnReceiveHandle(playerId, transmitter, 0, 0);
    }
    
    override protected event void OnReceive(int playerId, bool isSenderEditor, BaseTransceiver receiver, int frequency, float quality)
    {		
        OnReceiveHandle(playerId, receiver, frequency, quality);
    }
    
    protected void OnReceiveHandle(int playerId, BaseTransceiver receiver, int frequency, float quality)
    {
        if (!IsPlayerSpeech(playerId))
        {
            GetGame().GetCallqueue().Call(AwaitReceiveEnd, playerId);
        }
        bool alreadyReceive = IsPlayerSpeech(playerId);
        m_fPlayerSpeechReceiveTime[playerId] = GetGame().GetWorld().GetWorldTime() + 100;
        
        if (frequency == 32000) {
            bool isChannel = m_fPlayerSpeechReceiveIsChannel[playerId];
            m_fPlayerSpeechReceiveIsChannel[playerId] = true;
            if (!alreadyReceive || !isChannel)
                m_ScriptInvokerOnReceiveStart.Invoke(playerId, receiver, frequency, quality);
        }
        else {
            bool isChannel = m_fPlayerSpeechReceiveIsChannel[playerId];
            m_fPlayerSpeechReceiveIsChannel[playerId] = false;
            if (!alreadyReceive || isChannel)
                m_ScriptInvokerOnReceiveStart.Invoke(playerId, receiver, frequency, quality);
        }
    }
    
    void AwaitReceiveEnd(int playerId)
    {
        if (IsPlayerSpeech(playerId))
        {
            GetGame().GetCallqueue().Call(AwaitReceiveEnd, playerId);
            return;
        }
        
        m_ScriptInvokerOnReceiveEnd.Invoke(playerId);
    }
};