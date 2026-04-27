modded class SCR_VonDisplay
{
    // Наш статический список, к которому мы будем обращаться из лобби
    static ref set<int> s_LobbyActiveTalkers = new set<int>();

    // Перехватываем стандартный метод обновления ванильного VON
    override void DisplayUpdate(IEntity owner, float timeSlice)
    {
        // 1. Сначала даем ванильному коду отработать (он обновляет таймеры, убирает тех, кто замолчал)
        super.DisplayUpdate(owner, timeSlice);

        // 2. Очищаем наш временный список
        s_LobbyActiveTalkers.Clear();

        // 3. Собираем ID всех, кто сейчас реально говорит (m_bIsActive выставляется ванилью)
        foreach (TransmissionData data : m_aTransmissions)
        {
            if (data && data.m_bIsActive && data.m_iPlayerID != 0)
            {
                s_LobbyActiveTalkers.Insert(data.m_iPlayerID);
            }
        }
    }

    // Статический метод, который мы вызываем из LobbyMenu
    static bool IsPlayerTalking(int playerId)
    {
        return s_LobbyActiveTalkers.Contains(playerId);
    }
}