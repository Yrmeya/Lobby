// Маркеры синхронизируются штатной системой SCR_MapMarkerSyncComponent,
// игроки фракции видят точки своей фракции.
[BaseContainerProps()]
modded class SCR_MapMarkerSyncComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_iPlacedMarkerLimit = 100;
	}
}

// Ванильные маркеры работают штатно: виджеты создаются под вложенным
// "MapFrame" внутри корня карты (см. Melis_LobbyScreen_v2.layout).
modded class SCR_MapMarkerManagerComponent
{
	override void Update(float timeSlice)
	{
		super.Update(timeSlice);
	}
}

// Публичные маркеры (isLocal=false) идут через синк-RPC: сервер добавляет
// маркер и рассылает его клиентам, и именно в обработчике рассылки создаётся
// виджет. В оффлайн-сессии воркбенча broadcast не возвращается к отправителю
// (та же проблема, что с нашими точками) - виджеты не создаются, и менеджер
// падает с m_wRoot=null. Поэтому в одиночной сессии ставим маркеры как
// локальные: виджет создаётся сразу, без RPC-круга.
modded class SCR_MapMarkersUI
{
	override protected void OnInsertMarker(bool isLocal)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		ref array<int> players = {};
		if (pm) pm.GetPlayers(players);
		if (players.Count() <= 1)
			isLocal = true;

		super.OnInsertMarker(isLocal);
	}
}
