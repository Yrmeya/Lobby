class LobbyMenu : MenuBase
{
    static const int MAX_SQUADS           = 8;
    static const int MAX_ROLES_PER_SQUAD  = 8;
    static const int MAX_PLAYERS          = 32;
    static const int UI_TICK_MS           = 200;
    static const int FULL_SYNC_INTERVAL   = 10;

static const string LAYOUT_SQUAD_GROUP = "{0FE7E940AE42D291}UI/layouts/LobbySquadGroup.layout";
    static const string LAYOUT_ROLE_BUTTON = "{4BCFF556DEC3DE99}UI/layouts/LobbyRoleButton.layout";
    static const string LAYOUT_PLAYER_ROW  = "{71C9D1186F4BC866}UI/layouts/LobbyPlayerRow.layout";
    static const string LAYOUT_MAP_BACK_BUTTON = "{AA0001B200000001}UI/layouts/LobbyBackButton.layout";
    static const string LAYOUT_MAP_PLAYER_LIST = "{AA0001B200000002}UI/layouts/LobbyMapPlayerList.layout";
    static const string LAYOUT_MAP_POINT = "{AA0001B300000001}UI/layouts/LobbyMapPoint.layout";

    static const string LOBBY_MAP_CONFIG = "{0FDDAF3EB9EB45BD}Configs/Map/LobbyV2Map.conf";
    static const string VANILLA_MAP_CONFIG = "{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf";

    protected static LobbyMenu s_Instance;

    protected TextWidget   m_wStatusText;
    protected TextWidget   m_wPlayerCountText;
    protected TextWidget   m_wGMHint;
    protected TextWidget   m_wMissionDescription;
    protected ButtonWidget m_wStartButton;
    protected Widget       m_wRoleVBox;
    protected Widget       m_wPlayerVBox;

    protected Widget       m_wOpenMapButton;
    protected Widget       m_wBackToLobbyButton;
    protected Widget       m_wMapFrame;
    protected Widget       m_wMapBackButton;
    protected Widget       m_wMapPlayerPanel;
    protected Widget       m_wMapPlayerVBox;
    protected Widget       m_wTestPoint;
    protected SCR_MapEntity m_MapEntity;
    protected bool         m_bMapOpened;
    protected int          m_iMapOverlayTries;
    protected ref array<Widget> m_aLobbyPanels = {};
    protected ref array<Widget> m_aMapPlayerRows = {};
    protected ref array<Widget> m_aMapPointWidgets = {};
    protected ref array<float> m_aMapPointWX = {};
    protected ref array<float> m_aMapPointWY = {};
    protected bool m_bMapPointDown;
    protected int  m_iMapPointDownX, m_iMapPointDownY;
    protected int  m_iMapPointDiagLeft = 6;

    protected ref array<Widget> m_aPlayerRows = {};

    protected Widget       m_wChatHud;
    protected Widget       m_wChatPanel;
    protected SCR_ChatPanel m_ChatPanel;
    protected ButtonWidget      m_wChatOpenButton;
    protected SCR_ButtonBaseComponent m_ChatOpenButtonComponent;

    protected Widget m_wHoveredRoleFrame;

    protected bool m_bWasGM       = false;
    protected bool m_bStartPressed = false;
    protected int  m_iTick         = 0;
    protected bool m_bSquadsBuilt  = false;
    
    protected bool m_bLocalIsTalking = false;

    protected int m_iClickDiagLeft = 8;
    protected int m_iMouseDiagLeft = 8;

    // Сохраненные настройки громкости
    protected float m_fSavedSFX;
    protected float m_fSavedMusic;
    protected float m_fSavedDialog;
    protected float m_fSavedVoiceChat;

    protected ref map<int, string> m_mNames  = new map<int, string>();
    protected ref map<int, int>    m_mSquads = new map<int, int>();
    protected ref map<int, int>    m_mRoles  = new map<int, int>();

    static LobbyMenu GetInstance() { return s_Instance; }

    override void OnMenuOpen()
    {
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (mgr && !mgr.IsLobbyActive())
        {
            GetGame().GetCallqueue().CallLater(CloseSelf, 0, false);
            return;
        }

        s_Instance         = this;
        m_bStartPressed    = false;
        m_iTick            = 0;
        m_bSquadsBuilt     = false;
        m_wHoveredRoleFrame = null;
        m_bLocalIsTalking  = false; 

        m_mNames.Clear();
        m_mSquads.Clear();
        m_mRoles.Clear();

        Widget root = GetRootWidget();
        m_wStatusText      = TextWidget.Cast(root.FindAnyWidget("LobbyStatus"));
        m_wPlayerCountText = TextWidget.Cast(root.FindAnyWidget("PlayerCount"));
        m_wGMHint          = TextWidget.Cast(root.FindAnyWidget("GMHint"));
        m_wStartButton     = ButtonWidget.Cast(root.FindAnyWidget("StartGameButton"));
        m_wMissionDescription = TextWidget.Cast(root.FindAnyWidget("MissionDescription"));

        Widget roleContainer = root.FindAnyWidget("RoleListContainer");
        if (roleContainer) m_wRoleVBox = roleContainer.FindAnyWidget("RoleVBox");

        Widget playerContainer = root.FindAnyWidget("PlayerListContainer");
        if (playerContainer) m_wPlayerVBox = playerContainer.FindAnyWidget("PlayerVBox");

        m_wOpenMapButton = root.FindAnyWidget("OpenMapButton");
        m_wBackToLobbyButton = root.FindAnyWidget("BackToLobbyButton");
        m_wMapFrame = root.FindAnyWidget("MapFrame");
        m_MapEntity = null;
        m_bMapOpened = false;

        // Фрейм карты (ванильный Map.layout) нужен ванильному мап-коду, но
        // видим только пока карта открыта.
        if (m_wMapFrame)
            m_wMapFrame.SetVisible(false);

        m_aLobbyPanels.Clear();
        ref array<string> panelNames = {
            "HeaderFrame", "HeaderAccent", "PlayersHeader", "RolesHeader",
            "PlayerListContainer", "RoleListContainer", "MapPanel",
            "MapHint", "VoiceChatHint", "BottomStrip", "ChatOpenButton", "Chat"
        };
        foreach (string pn : panelNames)
        {
            Widget pw = root.FindAnyWidget(pn);
            if (pw)
                m_aLobbyPanels.Insert(pw);
        }

        if (m_wStartButton) 
        { 
            m_wStartButton.SetVisible(false); 
            m_wStartButton.SetOpacity(0.0); 
        }
        if (m_wGMHint) 
        { 
            m_wGMHint.SetVisible(false); 
            m_wGMHint.SetOpacity(0.0); 
        }

        BuildSquadGroups();
        BuildPlayerRows();
        SetupMissionDescription();

        m_wChatHud   = root.FindAnyWidget("Chat");
        m_wChatPanel = root.FindAnyWidget("ChatPanel");
        m_ChatPanel  = null;
        if (m_wChatPanel)
            m_ChatPanel = SCR_ChatPanel.Cast(m_wChatPanel.FindHandler(SCR_ChatPanel));
        // На сервере менеджер панелей отсутствует - чат не инициализирован,
        // прячем его, чтобы не висел пустым и не падал.
        if (m_wChatHud)
            m_wChatHud.SetVisible(m_ChatPanel != null && SCR_ChatPanelManager.GetInstance() != null);
        Print("[Lobby] Chat init: hud=" + (m_wChatHud != null) + " panelW=" + (m_wChatPanel != null) + " panel=" + (m_ChatPanel != null));

        m_wChatOpenButton = ButtonWidget.Cast(root.FindAnyWidget("ChatOpenButton"));
        m_ChatOpenButtonComponent = null;
        if (m_wChatOpenButton)
        {
            m_ChatOpenButtonComponent = SCR_ButtonBaseComponent.Cast(m_wChatOpenButton.FindHandler(SCR_ButtonBaseComponent));
            if (m_ChatOpenButtonComponent)
                m_ChatOpenButtonComponent.m_OnClicked.Insert(Action_ChatOpen);
        }

        InputManager inp = GetGame().GetInputManager();
        if (inp)
        {
            inp.AddActionListener("MenuBack",            EActionTrigger.DOWN, Action_Escape);
            inp.AddActionListener("LobbyGameForceStart", EActionTrigger.DOWN, Action_ForceStart);
            inp.AddActionListener("EditorToggle",        EActionTrigger.DOWN, Action_EditorToggle);
            
            inp.AddActionListener("LobbyVoN", EActionTrigger.DOWN, Action_LobbyVoNOn);
            inp.AddActionListener("LobbyVoN", EActionTrigger.UP, Action_LobbyVoNOff);
            inp.AddActionListener("LobbyMapOpen", EActionTrigger.DOWN, Action_ToggleMap);

            inp.AddActionListener("MouseLeft", EActionTrigger.DOWN, Action_MouseDiag);
        }

        GetGame().GetCallqueue().CallLater(OnTick, UI_TICK_MS, true);
        
        // Глушим окружение и делаем голос громким
        GetGame().GetCallqueue().CallLater(MuteEnvironment, 1500, false, true);
    }

    override void OnMenuClose()
    {
        CloseLobbyMap();
        // Возвращаем звук
        MuteEnvironment(false);

        GetGame().GetCallqueue().Remove(OnTick);
        GetGame().GetCallqueue().Remove(CloseSelf);

        InputManager inp = GetGame().GetInputManager();
        if (inp)
        {
            inp.RemoveActionListener("MenuBack",            EActionTrigger.DOWN, Action_Escape);
            inp.RemoveActionListener("LobbyGameForceStart", EActionTrigger.DOWN, Action_ForceStart);
            inp.RemoveActionListener("EditorToggle",        EActionTrigger.DOWN, Action_EditorToggle);
            
            inp.RemoveActionListener("LobbyVoN", EActionTrigger.DOWN, Action_LobbyVoNOn);
            inp.RemoveActionListener("LobbyVoN", EActionTrigger.UP, Action_LobbyVoNOff);
            inp.RemoveActionListener("LobbyMapOpen", EActionTrigger.DOWN, Action_ToggleMap);

            inp.RemoveActionListener("MouseLeft", EActionTrigger.DOWN, Action_MouseDiag);
        }

        if (m_ChatPanel)
        {
            SCR_ChatPanelManager pm = SCR_ChatPanelManager.GetInstance();
            if (pm) pm.CloseChatPanel(m_ChatPanel);
            m_ChatPanel = null;
        }

        m_aPlayerRows.Clear();
        m_mNames.Clear();
        m_mSquads.Clear();
        m_mRoles.Clear();

        m_wHoveredRoleFrame = null;
        m_bLocalIsTalking = false; 
        s_Instance = null;
    }

    protected bool m_bCtxDiagLogged;
    protected bool m_bViewportDiagLogged;

    override void OnMenuUpdate(float tDelta)
    {
        // На сервере менеджер панелей чата отсутствует, панель не
        // инициализирована - обновление падает с null.
        if (m_ChatPanel && SCR_ChatPanelManager.GetInstance())
            m_ChatPanel.OnUpdateChat(tDelta);
        super.OnMenuUpdate(tDelta);

        // Пока карта открыта, активируем контекст "MapContext", чтобы работали
        // зум (колесо) и панорамирование (перетаскивание) — рецепт из PlayableSelector
        // (PS_SpectatorMenu.OnMenuUpdate).
        if (m_MapEntity && m_MapEntity.IsOpen())
        {
            InputManager inp = GetGame().GetInputManager();
            if (inp && !inp.IsContextActive("MapContext"))
                inp.ActivateContext("MapContext");

            if (!m_bCtxDiagLogged)
            {
                m_bCtxDiagLogged = true;
                Print("[LobbyMap] MapContext active=" + inp.IsContextActive("MapContext") + " mapOpen=" + m_MapEntity.IsOpen() + " mapMode=" + m_MapEntity.GetMapConfig().MapEntityMode);
            }
        }
    } 

    override void OnMenuFocusGained()
    {
        super.OnMenuFocusGained();
    }

    override void OnMenuFocusLost()
    {
        Action_LobbyVoNOff();
        super.OnMenuFocusLost();
    }

    protected void OnTick()
    {
        m_iTick++;
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (mgr && !mgr.IsLobbyActive()) 
        { 
            CloseSelf(); 
            return; 
        }

        if (Replication.IsServer()) 
            SyncFromManager();
        else
        {
            bool noData  = m_mNames.IsEmpty();
            bool periodic = ((m_iTick % FULL_SYNC_INTERVAL) == 1);
            if (noData || periodic)
            {
                LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
                if (rpc) rpc.RequestFullSync();
            }
        }

        CheckGMVisibility();
        UpdateHeader();
        RefreshUI();

        if (m_bMapOpened)
        {
            RefreshMapPlayerList();
            UpdateMapPoints();
        }
		  
        // =====================================================================
        // ПРИНУДИТЕЛЬНОЕ УДЕРЖАНИЕ ТИШИНЫ
        // Движок Wwise постоянно пытается вернуть звук ветра. 
        // Если лобби открыто, мы каждый кадр сбрасываем его обратно в 0.
        // =====================================================================

    }

    protected void SyncFromManager()
    {
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (!mgr) return;
        
        foreach (int pid, LobbyPlayerData data : mgr.GetPlayersMap())
        {
            m_mNames.Set(pid, data.m_sPlayerName);
            m_mSquads.Set(pid, data.m_iSquadIndex);
            m_mRoles.Set(pid, data.m_iRoleIndex);
        }
        
        ref array<int> toRemove = new array<int>();
        foreach (int pid, string nm : m_mNames) 
        { 
            if (!mgr.GetPlayerData(pid)) 
                toRemove.Insert(pid); 
        }
        
        foreach (int pid : toRemove) 
        { 
            m_mNames.Remove(pid); 
            m_mSquads.Remove(pid); 
            m_mRoles.Remove(pid); 
        }
    }

    protected void CloseSelf() 
    { 
        GetGame().GetMenuManager().CloseMenu(this); 
    }

    // =====================================================================
    // ОПИСАНИЕ МИССИИ (нижний блок)
    // =====================================================================
    protected void SetupMissionDescription()
    {
        if (!m_wMissionDescription) return;

        MissionHeader header = GetGame().GetMissionHeader();
        if (!header) return;

        SCR_MissionHeader scrHeader = SCR_MissionHeader.Cast(header);
        if (!scrHeader) return;

        string desc = scrHeader.m_sDescription;
        if (desc.IsEmpty())
            desc = scrHeader.m_sDetails;
        if (desc.IsEmpty())
            return;

        m_wMissionDescription.SetText(desc);
    }
    // =====================================================================


    // =====================================================================
    // ПОЛНОЭКРАННАЯ КАРТА ЛОББИ (открывается кнопкой или клавишей M)
    // =====================================================================
    protected void Action_ToggleMap()
    {
        if (m_bMapOpened)
            CloseLobbyMap();
        else
            OpenLobbyMapFullscreen();
    }

    protected void Action_MapBackButton(SCR_ButtonBaseComponent button)
    {
        Action_ToggleMap();
    }

    protected void OpenLobbyMapFullscreen()
    {
        if (!m_MapEntity)
            m_MapEntity = SCR_MapEntity.GetMapInstance();
        if (!m_MapEntity)
            return;

        // Ванильный полноэкранный режим с ванильным конфигом: здесь всё
        // работает как в обычной игре (колесо, панорама, маркеры).
        // Ванильный мап-код ищет свои виджеты (MapWidget, CrossGrid и т.п.)
        // в дереве меню - они лежат в MapFrame (Map.layout).
        if (m_wMapFrame)
            m_wMapFrame.SetVisible(true);

        Widget rootRef = m_wMapFrame;
        if (!rootRef)
            rootRef = GetRootWidget();

        MapConfiguration mapConfig = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, LOBBY_MAP_CONFIG, rootRef);
        if (!mapConfig)
            return;

        mapConfig.MapEntityMode = EMapEntityMode.PLAIN;
        m_MapEntity.OpenMap(mapConfig);
        m_MapEntity.CloseMap();
        mapConfig.MapEntityMode = EMapEntityMode.FULLSCREEN;
        m_MapEntity.OpenMap(mapConfig);

        GetGame().GetCallqueue().CallLater(ZoomMapOut, 200, false);

        // Оверлеи карты (кнопка возврата + список игроков) создаём поверх
        // корня карты, когда она готова (открывается не мгновенно).
        m_iMapOverlayTries = 0;
        GetGame().GetCallqueue().CallLater(CreateMapOverlays, 100, false);

        m_bMapOpened = true;
        SetLobbyPanelsVisible(false);
        if (m_wBackToLobbyButton)
            m_wBackToLobbyButton.SetVisible(true);
        Print("[LobbyMap] fullscreen map opened");
    }

    protected void CreateMapOverlays()
    {
        if (!m_bMapOpened || !m_MapEntity || !m_MapEntity.IsOpen())
            return;

        // Ванильный полноэкранный мап-экран висит в корне HUD (m_wRootTop)
        // и перекрывает меню, поэтому оверлеи создаём ПОВЕРХ всего -
        // в корне HUD (или в корне workspace как запасной вариант).
        WorkspaceWidget ws = GetGame().GetWorkspace();
        Widget hudTop = ws.FindAnyWidget("SCR_HUDManagerComponent.m_wRootTop");
        Widget overlayParent = ws;
        if (hudTop)
            overlayParent = hudTop;

        // Слот корня созданного лейаута движком не применяется (всё встаёт
        // в 0x0) - позиционируем оверлеи вручную.
        int screenW = ws.GetWidth();
        int screenH = ws.GetHeight();

        if (!m_wMapBackButton)
        {
            m_wMapBackButton = ws.CreateWidgets(LAYOUT_MAP_BACK_BUTTON, overlayParent);
            if (m_wMapBackButton)
            {
                Widget backBtn = m_wMapBackButton.FindAnyWidget("BackButton");
                if (backBtn)
                {
                    SCR_ButtonBaseComponent comp = SCR_ButtonBaseComponent.Cast(backBtn.FindHandler(SCR_ButtonBaseComponent));
                    if (comp)
                        comp.m_OnClicked.Insert(Action_MapBackButton);
                }

                int bw = screenW / 5;
                FrameSlot.SetPosX(m_wMapBackButton, ws.DPIUnscale(screenW / 2 - bw / 2));
                FrameSlot.SetPosY(m_wMapBackButton, ws.DPIUnscale(12));
                FrameSlot.SetSizeX(m_wMapBackButton, ws.DPIUnscale(bw));
                FrameSlot.SetSizeY(m_wMapBackButton, ws.DPIUnscale(36));

                float bx, by, bsx, bsy;
                m_wMapBackButton.GetScreenPos(bx, by);
                m_wMapBackButton.GetScreenSize(bsx, bsy);
                Print("[LobbyMap] back button rect pos=(" + bx + "," + by + ") size=(" + bsx + "," + bsy + ") parent='" + overlayParent.GetName() + "'");
            }
        }

        if (!m_wMapPlayerPanel)
        {
            m_wMapPlayerPanel = ws.CreateWidgets(LAYOUT_MAP_PLAYER_LIST, overlayParent);
            if (m_wMapPlayerPanel)
            {
                m_wMapPlayerVBox = m_wMapPlayerPanel.FindAnyWidget("MapPlayerVBox");
                m_aMapPlayerRows.Clear();
                for (int i = 0; i < MAX_PLAYERS; i++)
                {
                    if (!m_wMapPlayerVBox) break;
                    Widget row = ws.CreateWidgets(LAYOUT_PLAYER_ROW, m_wMapPlayerVBox);
                    if (!row) break;
                    row.SetName("MPROW_" + i);
                    row.SetVisible(false);
                    m_aMapPlayerRows.Insert(row);
                }

                FrameSlot.SetPosX(m_wMapPlayerPanel, ws.DPIUnscale((int)(screenW * 0.05)));
                FrameSlot.SetPosY(m_wMapPlayerPanel, ws.DPIUnscale(150));
                FrameSlot.SetSizeX(m_wMapPlayerPanel, ws.DPIUnscale((int)(screenW * 0.23)));
                FrameSlot.SetSizeY(m_wMapPlayerPanel, ws.DPIUnscale(screenH - 190));

                float px, py, psx, psy;
                m_wMapPlayerPanel.GetScreenPos(px, py);
                m_wMapPlayerPanel.GetScreenSize(psx, psy);
                Print("[LobbyMap] player panel rect pos=(" + px + "," + py + ") size=(" + psx + "," + psy + ")");
            }
        }

        Print("[LobbyMap] map overlays created");
    }

    protected void CloseLobbyMap()
    {
        if (m_bMapOpened && m_MapEntity)
            m_MapEntity.CloseMap();
        m_bMapOpened = false;

        if (m_wMapBackButton)
        {
            m_wMapBackButton.RemoveFromHierarchy();
            m_wMapBackButton = null;
        }
        if (m_wMapPlayerPanel)
        {
            m_wMapPlayerPanel.RemoveFromHierarchy();
            m_wMapPlayerPanel = null;
        }
        if (m_wTestPoint)
        {
            m_wTestPoint.RemoveFromHierarchy();
            m_wTestPoint = null;
        }
        m_wMapPlayerVBox = null;
        m_aMapPlayerRows.Clear();
        ClearMapPoints();

        SetLobbyPanelsVisible(true);
        if (m_wBackToLobbyButton)
            m_wBackToLobbyButton.SetVisible(false);
        if (m_wMapFrame)
            m_wMapFrame.SetVisible(false);
        Print("[LobbyMap] fullscreen map closed");
    }

    protected void SetLobbyPanelsVisible(bool visible)
    {
        foreach (Widget w : m_aLobbyPanels)
        {
            if (w)
                w.SetVisible(visible);
        }
    }

    protected void ZoomMapOut()
    {
        if (m_MapEntity && m_MapEntity.IsOpen())
            m_MapEntity.ZoomOut();
    }
    // =====================================================================

    protected void BuildSquadGroups()
    {
        if (!m_wRoleVBox) return;
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (!mgr) return;
        WorkspaceWidget ws = GetGame().GetWorkspace();
        array<ref LobbySquadConfig> squads = mgr.GetSquads();

        for (int si = 0; si < squads.Count() && si < MAX_SQUADS; si++)
        {
            Widget group = ws.CreateWidgets(LAYOUT_SQUAD_GROUP, m_wRoleVBox);
            if (!group) continue;
            group.SetName("SQ_" + si);
            
            TextWidget nameW = TextWidget.Cast(group.FindAnyWidget("SquadName"));
            if (nameW) 
            {
                nameW.SetText(squads[si].m_sSquadName);
                Color squadColor = ParseHexColor(squads[si].m_sColor);
                nameW.SetColor(squadColor);
            }
            
            Widget roleList = group.FindAnyWidget("RoleList");
            if (!roleList) roleList = group;

            array<ref LobbyRoleConfig> roles = squads[si].GetRoles();
            for (int ri = 0; ri < roles.Count() && ri < MAX_ROLES_PER_SQUAD; ri++)
            {
                Widget frame = ws.CreateWidgets(LAYOUT_ROLE_BUTTON, roleList);
                if (!frame) continue;
                frame.SetName("ROLE_" + si + "_" + ri);
                TextWidget label     = TextWidget.Cast(frame.FindAnyWidget("Label"));
                TextWidget slotInfo  = TextWidget.Cast(frame.FindAnyWidget("SlotInfo"));
                TextWidget occupiedW = TextWidget.Cast(frame.FindAnyWidget("OccupiedBy"));
                if (label)     label.SetText(roles[ri].m_sRoleName);
                if (slotInfo)  slotInfo.SetText("0/" + roles[ri].m_iMaxPlayers);
                if (occupiedW) occupiedW.SetText("");
            }
        }
        m_bSquadsBuilt = true;
    }

    protected void BuildPlayerRows()
    {
        if (!m_wPlayerVBox) return;
        WorkspaceWidget ws = GetGame().GetWorkspace();
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            Widget row = ws.CreateWidgets(LAYOUT_PLAYER_ROW, m_wPlayerVBox);
            if (!row) break;
            row.SetName("PROW_" + i);
            row.SetVisible(false);
            m_aPlayerRows.Insert(row);
        }
    }

    void OnPlayerJoined(int playerId, string playerName)
    {
        m_mNames.Set(playerId, playerName);
        if (!m_mSquads.Contains(playerId)) m_mSquads.Set(playerId, -1);
        if (!m_mRoles.Contains(playerId))  m_mRoles.Set(playerId, -1);
    }

    void OnPlayerLeft(int playerId)
    {
        m_mNames.Remove(playerId); 
        m_mSquads.Remove(playerId); 
        m_mRoles.Remove(playerId);
    }

    void OnPlayerAssigned(int playerId, int squadIndex, int roleIndex)
    {
        if (!m_mNames.Contains(playerId)) 
            m_mNames.Set(playerId, "Player " + playerId);
        m_mSquads.Set(playerId, squadIndex); 
        m_mRoles.Set(playerId, roleIndex);
    }

    void OnHeaderUpdated(int playerCount, int readyCount)
    {
        if (m_wPlayerCountText) 
            m_wPlayerCountText.SetText("Players: " + playerCount + " | Ready: " + readyCount);
    }

    protected void CheckGMVisibility()
    {
        bool isGM = IsLocalPlayerGM();
        if (m_wStartButton && !m_bStartPressed)
        {
            m_wStartButton.SetVisible(isGM);
            if (isGM)
                m_wStartButton.SetOpacity(1.0);
            else
                m_wStartButton.SetOpacity(0.0);
        }
        if (m_wGMHint)
        {
            m_wGMHint.SetVisible(isGM);
            if (isGM)
                m_wGMHint.SetOpacity(1.0);
            else
                m_wGMHint.SetOpacity(0.0);
        }
        m_bWasGM = isGM;
    }

    protected void UpdateHeader()
    {
        if (m_wPlayerCountText) 
            m_wPlayerCountText.SetText("Players: " + m_mNames.Count());
    }

    protected void RefreshUI()
    {
        if (!m_bSquadsBuilt) return;
        RefreshRoleUI();
        RefreshPlayerList();
    }

    protected void RefreshRoleUI()
    {
        if (!m_wRoleVBox) return;
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (!mgr) return;
        
        int localId = GetLocalPlayerId();
        int mySquad = -1, myRole = -1;
        if (m_mSquads.Contains(localId)) mySquad = m_mSquads.Get(localId);
        if (m_mRoles.Contains(localId))  myRole  = m_mRoles.Get(localId);
        array<ref LobbySquadConfig> squads = mgr.GetSquads();

        for (int si = 0; si < squads.Count() && si < MAX_SQUADS; si++)
        {
            Widget group = m_wRoleVBox.FindAnyWidget("SQ_" + si);
            if (!group) continue;
            
            array<ref LobbyRoleConfig> roles = squads[si].GetRoles();
            for (int ri = 0; ri < roles.Count() && ri < MAX_ROLES_PER_SQUAD; ri++)
            {
                Widget frame = group.FindAnyWidget("ROLE_" + si + "_" + ri);
                if (!frame) continue;
                
                int occupied = 0; 
                string occupantName = "";
                
                foreach (int pid, int psq : m_mSquads)
                {
                    int prl = -1; 
                    if (m_mRoles.Contains(pid)) prl = m_mRoles.Get(pid);
                    if (psq == si && prl == ri) 
                    { 
                        occupied++; 
                        if (m_mNames.Contains(pid)) 
                        { 
                            string pn = m_mNames.Get(pid); 
                            if (pn != "") 
                                occupantName = pn; 
                        } 
                    }
                }
                
                int  maxSlots = roles[ri].m_iMaxPlayers;
                bool isMine   = (si == mySquad && ri == myRole);
                bool isFull   = (occupied >= maxSlots) && !isMine;
                bool isHovered = (frame == m_wHoveredRoleFrame);

                TextWidget slotInfo = TextWidget.Cast(frame.FindAnyWidget("SlotInfo"));
                if (slotInfo) 
                    slotInfo.SetText("" + occupied + "/" + maxSlots);
                    
                TextWidget occupiedByW = TextWidget.Cast(frame.FindAnyWidget("OccupiedBy"));
                if (occupiedByW) 
                { 
                    occupiedByW.SetText(occupantName); 
                    if (occupantName != "")
                        occupiedByW.SetColor(new Color(0.53, 0.68, 0.5, 1.0));
                    else
                        occupiedByW.SetColor(new Color(0.45, 0.47, 0.45, 1.0));
                }
                
                Widget bg = frame.FindAnyWidget("VisualBackground");
                if (bg)
                {
                    if (isFull)
                        bg.SetOpacity(0.5);
                    else
                        bg.SetOpacity(1.0);
                }

                Widget btn = frame.FindAnyWidget("RoleButton");
                if (btn)
                {
                    if (isMine)
                        btn.SetColor(new Color(0.45, 0.36, 0.18, 0.6));
                    else if (isHovered)
                        btn.SetColor(new Color(0.42, 0.38, 0.27, 0.5));
                    else if (isFull)
                        btn.SetColor(new Color(0.3, 0.16, 0.12, 0.5));
                    else
                        btn.SetColor(new Color(0.08, 0.09, 0.1, 0.45));
                }
            }
        }
    }

    protected void RefreshPlayerList()
    {
        if (!m_wPlayerVBox || m_aPlayerRows.IsEmpty()) return;
        PopulatePlayerRows(m_aPlayerRows);
    }

    protected void RefreshMapPlayerList()
    {
        if (!m_wMapPlayerVBox || m_aMapPlayerRows.IsEmpty()) return;
        PopulatePlayerRows(m_aMapPlayerRows);
    }

    protected void PopulatePlayerRows(inout array<Widget> rows)
    {
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        int localId = GetLocalPlayerId();

        ref array<int> withRole = new array<int>(); 
        ref array<int> withoutRole = new array<int>();
        
        foreach (int pid, string pname : m_mNames)
        {
            int psq = -1, prl = -1;
            if (m_mSquads.Contains(pid)) psq = m_mSquads.Get(pid);
            if (m_mRoles.Contains(pid))  prl = m_mRoles.Get(pid);
            if (psq >= 0 && prl >= 0) 
                withRole.Insert(pid); 
            else 
                withoutRole.Insert(pid);
        }
        
        int rowIdx = 0;
        foreach (int pid1 : withRole)
        {
            if (rowIdx >= rows.Count()) break;
            int psq1 = -1, prl1 = -1;
            if (m_mSquads.Contains(pid1)) psq1 = m_mSquads.Get(pid1);
            if (m_mRoles.Contains(pid1))  prl1 = m_mRoles.Get(pid1);
            string sqN = "--", rlN = "--";
            if (mgr) 
            { 
                sqN = mgr.GetSquadName(psq1); 
                rlN = mgr.GetRoleName(psq1, prl1); 
            }
            Widget row1 = rows[rowIdx]; 
            row1.SetVisible(true); 
            SetPlayerRow(row1, m_mNames.Get(pid1), sqN, rlN, pid1 == localId, true, pid1); 
            rowIdx++;
        }
        foreach (int pid2 : withoutRole)
        {
            if (rowIdx >= rows.Count()) break;
            Widget row2 = rows[rowIdx]; 
            row2.SetVisible(true); 
            SetPlayerRow(row2, m_mNames.Get(pid2), "--", "--", pid2 == localId, false, pid2); 
            rowIdx++;
        }
        for (int i = rowIdx; i < rows.Count(); i++) 
            rows[i].SetVisible(false);
    }

    protected void SetPlayerRow(Widget row, string playerName, string squadName, string roleName, bool isMe, bool hasRole, int playerId)
    {
        bool isTalking = (isMe && m_bLocalIsTalking)
            || SCR_VonDisplay.IsPlayerTalking(playerId)
            || SCR_VoNComponent.LobbyIsPlayerTalking(playerId);

        Widget vonIndicator = row.FindAnyWidget("VONIndicator");
        
        if (vonIndicator)
        {
            if (isTalking)
                vonIndicator.SetColor(new Color(0.95, 0.25, 0.2, 1.0)); 
            else
                vonIndicator.SetColor(new Color(0.13, 0.13, 0.13, 1.0)); 
        }

        Color nameColor;
        if (isTalking)
            nameColor = new Color(0.95, 0.3, 0.25, 1.0);
        else if (isMe)
            nameColor = new Color(0.6, 0.76, 0.85, 1.0); 
        else if (hasRole)
            nameColor = new Color(0.85, 0.86, 0.82, 0.9); 
        else
            nameColor = new Color(0.5, 0.52, 0.5, 1.0);  

        Color infoColor;
        if (hasRole)
            infoColor = new Color(0.58, 0.6, 0.56, 1.0);
        else
            infoColor = new Color(0.44, 0.46, 0.44, 1.0);

        TextWidget nameW = TextWidget.Cast(row.FindAnyWidget("PlayerName"));
        TextWidget sqW   = TextWidget.Cast(row.FindAnyWidget("SquadLabel"));
        TextWidget rlW   = TextWidget.Cast(row.FindAnyWidget("RoleLabel"));
        
        if (nameW) { nameW.SetText(playerName); nameW.SetColor(nameColor); }
        if (sqW)   { sqW.SetText(squadName);    sqW.SetColor(infoColor);   }
        if (rlW)   { rlW.SetText(roleName);     rlW.SetColor(infoColor);   }
    }

    protected Widget FindRoleFrame(Widget w)
    {
        Widget cur = w;
        while (cur) 
        { 
            if (cur.GetName().StartsWith("ROLE_")) 
                return cur; 
            cur = cur.GetParent(); 
        }
        return null;
    }

    override bool OnMouseEnter(Widget w, int x, int y) 
    { 
        Widget f = FindRoleFrame(w); 
        if (f) 
        {
m_wHoveredRoleFrame = f;
            Widget overlay = f.FindAnyWidget("HoverOverlay");
            if (overlay) overlay.SetColor(new Color(0.9, 0.75, 0.45, 0.12));
        }
        return false; 
    }
    
    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y) 
    { 
        Widget l = FindRoleFrame(w); 
        if (l && l == m_wHoveredRoleFrame) 
        {
            m_wHoveredRoleFrame = null;
            Widget overlay = l.FindAnyWidget("HoverOverlay");
            if (overlay) overlay.SetColor(new Color(1, 1, 1, 0)); 
        }
        return false; 
    }
    
    override bool OnFocus(Widget w, int x, int y) 
    { 
        Widget f = FindRoleFrame(w); 
        if (f) 
        {
            m_wHoveredRoleFrame = f;
            Widget overlay = f.FindAnyWidget("HoverOverlay");
            if (overlay) overlay.SetColor(new Color(0.9, 0.75, 0.45, 0.12));
        }
        return false; 
    }
    
    override bool OnFocusLost(Widget w, int x, int y) 
    { 
        Widget l = FindRoleFrame(w); 
        if (l && l == m_wHoveredRoleFrame) 
        {
            m_wHoveredRoleFrame = null;
            Widget overlay = l.FindAnyWidget("HoverOverlay");
            if (overlay) overlay.SetColor(new Color(1, 1, 1, 0));
        }
        return false; 
    }

    protected void Action_MouseDiag()
    {
        // Для постановки точек запоминаем точку нажатия.
        if (m_bMapOpened && m_MapEntity && m_MapEntity.IsOpen() && IsCursorOverMap())
        {
            WidgetManager.GetMousePos(m_iMapPointDownX, m_iMapPointDownY);
            m_bMapPointDown = true;
        }

        if (m_iMouseDiagLeft <= 0) return;
        m_iMouseDiagLeft--;

        int mouseX, mouseY;
        WidgetManager.GetMousePos(mouseX, mouseY);
        array<Widget> outWidgets = {};
        WidgetManager.TraceWidgets(mouseX, mouseY, GetGame().GetWorkspace(), outWidgets);
        Print("[LobbyMouse] LMB at (" + mouseX + ", " + mouseY + "), widgets under cursor: " + outWidgets.Count());
        int shown = 0;
        foreach (Widget w : outWidgets)
        {
            if (shown >= 6) break;
            Print("[LobbyMouse]   #" + shown + " class='" + w.ClassName() + "' name='" + w.GetName() + "' vis=" + w.IsVisible());
            shown++;
        }
    }

    //------------------------------------------------------------------------------------------------
    //! ЛКМ отпущена: короткий клик над картой = постановка точки.
    protected void Action_MapPointUp()
    {
        if (!m_bMapPointDown)
            return;
        m_bMapPointDown = false;

        if (!m_bMapOpened || !m_MapEntity || !m_MapEntity.IsOpen())
            return;

        int upX, upY;
        WidgetManager.GetMousePos(upX, upY);
        int moved = Math.AbsInt(upX - m_iMapPointDownX) + Math.AbsInt(upY - m_iMapPointDownY);
        if (moved > 8)
            return;

        float wX, wY;
        m_MapEntity.GetMapCursorWorldPosition(wX, wY);
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc)
            rpc.RequestPlaceMapPoint(wX, wY);

        if (m_iMapPointDiagLeft > 0)
        {
            m_iMapPointDiagLeft--;
            Print("[LobbyMap] map point requested at world (" + wX + ", " + wY + ")");
        }
    }

    protected bool IsCursorOverMap()
    {
        if (!m_MapEntity) return false;
        CanvasWidget mapWidget = m_MapEntity.GetMapWidget();
        if (!mapWidget) return false;

        int mouseX, mouseY;
        WidgetManager.GetMousePos(mouseX, mouseY);
        array<Widget> outWidgets = {};
        WidgetManager.TraceWidgets(mouseX, mouseY, GetGame().GetWorkspace(), outWidgets);
        foreach (Widget w : outWidgets)
        {
            if (w == mapWidget)
                return true;
        }
        return false;
    }

    //------------------------------------------------------------------------------------------------
    //! Получена точка карты (RPC): рисуем виджет поверх карты.
    void OnMapPointReceived(int playerId, float worldX, float worldY)
    {
        Print("[LobbyMap] OnMapPointReceived pid=" + playerId + " mapOpened=" + m_bMapOpened + " entity=" + (m_MapEntity != null) + " open=" + ((m_MapEntity != null) && m_MapEntity.IsOpen()) + " mapWidget=" + ((m_MapEntity != null) && m_MapEntity.GetMapWidget() != null));

        if (!m_MapEntity || !m_MapEntity.IsOpen())
            return;

        CanvasWidget mapWidget = m_MapEntity.GetMapWidget();
        if (!mapWidget)
            return;

        // Канвас карты не принимает детей, а рендерится отдельным слоем,
        // перекрывающим виджеты фрейма. Поэтому точки вешаем в корень HUD
        // (там же живёт мап-экран) - это гарантированно поверх карты.
        WorkspaceWidget ws = GetGame().GetWorkspace();
        Widget hudTop = ws.FindAnyWidget("SCR_HUDManagerComponent.m_wRootTop");
        Widget pointParent = ws;
        if (hudTop)
            pointParent = hudTop;

        Widget point = ws.CreateWidgets(LAYOUT_MAP_POINT, pointParent);
        if (!point)
            return;

        m_aMapPointWidgets.Insert(point);
        m_aMapPointWX.Insert(worldX);
        m_aMapPointWY.Insert(worldY);
        UpdateMapPointPosition(point, worldX, worldY);
    }

    protected void UpdateMapPointPosition(Widget point, float worldX, float worldY)
    {
        if (!m_MapEntity) return;
        CanvasWidget mapWidget = m_MapEntity.GetMapWidget();
        if (!mapWidget) return;

        int sX, sY;
        m_MapEntity.WorldToScreen(worldX, worldY, sX, sY, true);

        Widget parent = point.GetParent();
        float mx = 0, my = 0;
        if (parent)
            parent.GetScreenPos(mx, my);

        int slotX = sX - (int)mx - 6;
        int slotY = sY - (int)my - 6;

        WorkspaceWidget ws = GetGame().GetWorkspace();

        // Слоты живут в DPI-unscaled пространстве - конвертируем.
        FrameSlot.SetPosX(point, ws.DPIUnscale(sX - (int)mx - 16));
        FrameSlot.SetPosY(point, ws.DPIUnscale(sY - (int)my - 16));
        FrameSlot.SetSizeX(point, ws.DPIUnscale(32));
        FrameSlot.SetSizeY(point, ws.DPIUnscale(32));

        if (m_iMapPointDiagLeft > 0)
        {
            m_iMapPointDiagLeft--;
            float pX, pY, pSX, pSY;
            point.GetScreenPos(pX, pY);
            point.GetScreenSize(pSX, pSY);
            Print("[LobbyMap] point pos: world=(" + worldX + "," + worldY + ") screen=(" + sX + "," + sY + ") slot=(" + slotX + "," + slotY + ") ownRect=(" + pX + "," + pY + ") size=(" + pSX + "," + pSY + ")");
        }
    }

    protected void UpdateMapPoints()
    {
        for (int i = 0; i < m_aMapPointWidgets.Count(); i++)
            UpdateMapPointPosition(m_aMapPointWidgets[i], m_aMapPointWX[i], m_aMapPointWY[i]);
    }

    protected void ClearMapPoints()
    {
        foreach (Widget point : m_aMapPointWidgets)
        {
            if (point)
                point.RemoveFromHierarchy();
        }
        m_aMapPointWidgets.Clear();
        m_aMapPointWX.Clear();
        m_aMapPointWY.Clear();
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (m_iClickDiagLeft > 0)
        {
            m_iClickDiagLeft--;
            Print("[LobbyClick] OnClick arrived: class='" + w.ClassName() + "' name='" + w.GetName() + "' button=" + button);
        }

        if (w == m_wStartButton) 
        { 
            Action_ForceStart(); 
            return true; 
        }

        if (w == m_wOpenMapButton)
        {
            Action_ToggleMap();
            return true;
        }

        if (w == m_wBackToLobbyButton)
        {
            Action_ToggleMap();
            return true;
        }

        if (m_wMapBackButton)
        {
            Widget cur = w;
            while (cur)
            {
                if (cur == m_wMapBackButton)
                {
                    Action_ToggleMap();
                    return true;
                }
                cur = cur.GetParent();
            }
        }
        
        Widget cur = w;
        while (cur)
        {
            string wname = cur.GetName();
            if (wname.StartsWith("ROLE_"))
            {
                string tail = wname.Substring(5, wname.Length() - 5); 
                int sep = -1;
                for (int c = 0; c < tail.Length(); c++) 
                { 
                    if (tail.Substring(c, 1) == "_") 
                    { 
                        sep = c; 
                        break; 
                    } 
                }
                if (sep >= 0) 
                { 
                    int si = tail.Substring(0, sep).ToInt(); 
                    int ri = tail.Substring(sep + 1, tail.Length() - sep - 1).ToInt(); 
                    OnRoleClicked(si, ri); 
                    return true; 
                }
            }
            cur = cur.GetParent();
        }
        return false;
    }

    protected void OnRoleClicked(int squadIndex, int roleIndex)
    {
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (!rpc) return;
        
        int localId = GetLocalPlayerId();
        int curSq = -1, curRl = -1;
        if (m_mSquads.Contains(localId)) curSq = m_mSquads.Get(localId);
        if (m_mRoles.Contains(localId))  curRl = m_mRoles.Get(localId);
        
        bool deselect = (curSq == squadIndex && curRl == roleIndex);
        if (deselect) 
        { 
            m_mSquads.Set(localId, -1); 
            m_mRoles.Set(localId, -1); 
            rpc.RequestSelectRole(-1, -1); 
        }
        else 
        { 
            m_mSquads.Set(localId, squadIndex); 
            m_mRoles.Set(localId, roleIndex); 
            rpc.RequestSelectRole(squadIndex, roleIndex); 
        }
    }

    // =====================================================================
    // ПЕРЕХВАТ PTT 
    // =====================================================================
    void Action_LobbyVoNOn()
    {
        Print("[LobbyVoN] PTT DOWN fired", LogLevel.DEBUG);
        SCR_PlayerController scrPc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
        if (scrPc) 
        {
            scrPc.LobbyVoNEnable();
            m_bLocalIsTalking = true; 
        } 
    }
    
    void Action_LobbyVoNOff()
    {
        Print("[LobbyVoN] PTT UP fired", LogLevel.DEBUG);
        SCR_PlayerController scrPc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
        if (scrPc) 
        {
            scrPc.LobbyVoNDisable();
        }
        
        m_bLocalIsTalking = false; 
    }
    // =====================================================================

    // =====================================================================
    // УПРАВЛЕНИЕ ЗВУКОМ ОКРУЖЕНИЯ (По API AudioSystem)
    // =====================================================================
    // =====================================================================
    // УПРАВЛЕНИЕ ЗВУКОМ ОКРУЖЕНИЯ (По API AudioSystem)
    // =====================================================================
     // =====================================================================
    // УПРАВЛЕНИЕ ЗВУКОМ ОКРУЖЕНИЯ (По логике мода Earplugs)
    // =====================================================================
    // =====================================================================
    // УПРАВЛЕНИЕ ЗВУКОМ (Точно по логике Earplugs мода)
    // =====================================================================
    void MuteEnvironment(bool mute)
    {
        UserSettings engineSettings = GetGame().GetEngineUserSettings();
        if (!engineSettings) return;
        
        BaseContainer audioSettings = engineSettings.GetModule("AudioSettings");
        if (!audioSettings) return;
        
        if (mute)
        {
            // Читаем текущие настройки из конфига (они в масштабе 0-100)
            float sfxVol = 100, musicVol = 100, vonVol = 100, dialogVol = 100;
            audioSettings.Get("VolumeSfx", sfxVol);
            audioSettings.Get("VolumeMusic", musicVol);
            audioSettings.Get("VolumeVoN", vonVol);
            audioSettings.Get("VolumeDialog", dialogVol);
            
            // Переводим в масштаб 0.0 - 1.0 и сохраняем
            m_fSavedSFX = sfxVol * 0.01;
            m_fSavedMusic = musicVol * 0.01;
            m_fSavedVoiceChat = vonVol * 0.01;
            m_fSavedDialog = dialogVol * 0.01;
            
            // Применяем тишину и громкий голос ОДИН РАЗ
            AudioSystem.SetMasterVolume(AudioSystem.SFX, 0.0);
            AudioSystem.SetMasterVolume(AudioSystem.Music, 0.0);
            AudioSystem.SetMasterVolume(AudioSystem.Dialog, 0.0);
            AudioSystem.SetMasterVolume(AudioSystem.VoiceChat, 1.0);
        }
        else
        {
            // Возвращаем всё как было
            AudioSystem.SetMasterVolume(AudioSystem.SFX, m_fSavedSFX);
            AudioSystem.SetMasterVolume(AudioSystem.Music, m_fSavedMusic);
            AudioSystem.SetMasterVolume(AudioSystem.VoiceChat, m_fSavedVoiceChat);
            AudioSystem.SetMasterVolume(AudioSystem.Dialog, m_fSavedDialog);
        }
    }
    // =====================================================================
    // =====================================================================
    // =====================================================================

    protected void Action_Escape()
    {
        // Если открыта карта - Esc закрывает её, а не меню паузы.
        if (m_bMapOpened)
        {
            CloseLobbyMap();
            return;
        }
        GetGame().GetCallqueue().CallLater(DoPauseMenu, 0);
    }
    protected void DoPauseMenu()       { ArmaReforgerScripted.OpenPauseMenu(); }

    protected void Action_ForceStart()
    {
        if (!IsLocalPlayerGM() || m_bStartPressed) return;
        m_bStartPressed = true;
        if (m_wStartButton) 
        { 
            m_wStartButton.SetEnabled(false); 
            m_wStartButton.SetOpacity(0.3); 
        }
        LobbyRPCComponent rpc = LobbyRPCComponent.GetInstance();
        if (rpc) rpc.RequestStartGame();
    }

    protected void Action_EditorToggle()
    {
        if (!IsLocalPlayerGM()) return;
        SCR_EditorManagerEntity em = SCR_EditorManagerEntity.GetInstance();
        if (!em) return;
        if (em.IsOpened()) 
            em.Close(); 
        else 
            em.Open();
    }

void ToggleChatPanel()
    {
        if (!m_ChatPanel) return;
        GetGame().GetCallqueue().CallLater(ToggleChatPanelWrap, 0);
    }

    void ToggleChatPanelWrap()
    {
        if (!m_ChatPanel) return;
        SCR_ChatPanelManager pm = SCR_ChatPanelManager.GetInstance();
        Print("[Lobby] ToggleChatPanel: open=" + m_ChatPanel.IsOpen() + " pm=" + (pm != null));
        if (!pm) return;
        if (m_ChatPanel.IsOpen())
            pm.CloseChatPanel(m_ChatPanel);
        else
            pm.OpenChatPanel(m_ChatPanel);
    }

    void Action_ChatOpen(SCR_ButtonBaseComponent button)
    {
        ToggleChatPanel();
    }

    protected int GetLocalPlayerId()
    {
        PlayerController pc = GetGame().GetPlayerController();
        if (!pc) return -1;
        return pc.GetPlayerId();
    }

    protected bool IsLocalPlayerGM()
    {
        SCR_EditorManagerEntity em = SCR_EditorManagerEntity.GetInstance();
        if (em) return !em.IsLimited();
        PlayerManager pm = GetGame().GetPlayerManager();
        if (!pm) return false;
        int localId = GetLocalPlayerId();
        ref array<int> ids = new array<int>();
        pm.GetPlayers(ids);
        if (ids.IsEmpty()) return false;
        return (localId == ids[0]);
    }

    protected static Color ParseHexColor(string hex)
    {
        if (hex.IsEmpty() || hex[0] != "#") return Color.White;
        hex.Replace("#", "");
        if (hex.Length() != 6) return Color.White;
        
        float r = (float)hex.Substring(0, 2).ToInt() / 255.0;
        float g = (float)hex.Substring(2, 2).ToInt() / 255.0;
        float b = (float)hex.Substring(4, 2).ToInt() / 255.0;
        return Color(r, g, b, 1.0);
    }
}