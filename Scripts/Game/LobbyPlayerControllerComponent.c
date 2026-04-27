[ComponentEditorProps(category: "Lobby", description: "Handles Lobby VoN logic on PlayerController")]
class LobbyPlayerControllerComponentClass : ScriptComponentClass {}

class LobbyPlayerControllerComponent : ScriptComponent
{
    protected SCR_VoNComponent m_VoN; // Ищем ванильный компонент!

    // Кэшируем ванильный VoN компонент на болванке
    SCR_VoNComponent GetVoN()
    {
        if (!m_VoN)
        {
            PlayerController pc = PlayerController.Cast(GetOwner());
            if (pc)
            {
                IEntity entity = pc.GetControlledEntity();
                if (entity)
                {
                    m_VoN = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));
                    
                    // Отладка
                    if (m_VoN)
                        Print("[LobbyVoN] SCR_VoNComponent found on dummy.", LogLevel.DEBUG);
                    else
                        Print("[LobbyVoN] ERROR: SCR_VoNComponent NOT found on controlled entity!", LogLevel.ERROR);
                }
                else
                {
                    Print("[LobbyVoN] ERROR: Controlled entity is NULL!", LogLevel.ERROR);
                }
            }
        }
        return m_VoN;
    }

    // Вызывается из LobbyMenu при нажатии кнопки PTT
    void LobbyVoNEnable()
    {
        Print("[LobbyVoN] LobbyVoNEnable called!", LogLevel.DEBUG);
        
        SCR_VoNComponent von = GetVoN();
        if (von)
        {
            // МАГИЯ ТУТ: Говорим ванильному компоненту начать захват микрофона
            von.SetCommMethod(ECommMethod.DIRECT);
            von.SetCapture(true);
            Print("[LobbyVoN] Microphone Capture ON", LogLevel.DEBUG);
        }
        else
        {
            Print("[LobbyVoN] Cannot enable VoN, component is missing.", LogLevel.ERROR);
        }
    }

    // Вызывается из LobbyMenu при отпускании кнопки PTT
    void LobbyVoNDisable()
    {
        SCR_VoNComponent von = GetVoN();
        if (von)
        {
            von.SetCapture(false);
            Print("[LobbyVoN] Microphone Capture OFF", LogLevel.DEBUG);
        }
    }
}