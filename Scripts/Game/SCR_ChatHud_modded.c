modded class SCR_ChatHud
{
	override protected void Callback_OnToggleAction()
	{
		Print("[Lobby] SCR_ChatHud::Callback_OnToggleAction fired");
		LobbyMenu lobby = LobbyMenu.GetInstance();
		if (lobby)
		{
			Print("[Lobby] SCR_ChatHud -> LobbyMenu.ToggleChatPanel");
			lobby.ToggleChatPanel();
			return;
		}
		Print("[Lobby] SCR_ChatHud -> vanilla toggle");
		super.Callback_OnToggleAction();
	}
}
