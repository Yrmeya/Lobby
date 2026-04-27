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

    protected static LobbyMenu s_Instance;

    protected TextWidget   m_wStatusText;
    protected TextWidget   m_wPlayerCountText;
    protected TextWidget   m_wGMHint;
    protected ButtonWidget m_wStartButton;
    protected Widget       m_wRoleVBox;
    protected Widget       m_wPlayerVBox;

    protected ref array<Widget> m_aPlayerRows = {};

    protected Widget m_wChatHudRoot;
    protected int    m_iChatHudOriginalZOrder;

    protected Widget m_wHoveredRoleFrame;

    protected bool m_bWasGM       = false;
    protected bool m_bStartPressed = false;
    protected int  m_iTick         = 0;
    protected bool m_bSquadsBuilt  = false;
    
    protected bool m_bLocalIsTalking = false;

    protected ref map<int, string> m_mNames  = new map<int, string>();
    protected ref map<int, int>    m_mSquads = new map<int, int>();
    protected ref map<int, int>    m_mRoles  = new map<int, int>();

    static LobbyMenu GetInstance() { return s_Instance; }

    override void OnMenuOpen()
    {
        Print("[LOBBY DEBUG] =====================================================================", LogLevel.WARNING);
        Print("[LOBBY DEBUG] === OnMenuOpen STARTED ===", LogLevel.WARNING);
        
        LobbyManagerComponent mgr = LobbyManagerComponent.GetInstance();
        if (mgr && !mgr.IsLobbyActive())
        {
            Print("[LOBBY DEBUG] ERROR: Lobby is NOT active! Menu will close now.", LogLevel.ERROR);
            GetGame().GetCallqueue().CallLater(CloseSelf, 0, false);
            return;
        }
        Print("[LOBBY DEBUG] Lobby is active. Proceeding with init...", LogLevel.WARNING);

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

        Widget roleContainer = root.FindAnyWidget("RoleListContainer");
        if (roleContainer) m_wRoleVBox = roleContainer.FindAnyWidget("RoleVBox");

        Widget playerContainer = root.FindAnyWidget("PlayerListContainer");
        if (playerContainer) m_wPlayerVBox = playerContainer.FindAnyWidget("PlayerVBox");

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
        RaiseChatHud();

        Print("[LOBBY DEBUG] Registering Input Listeners...", LogLevel.WARNING);
        InputManager inp = GetGame().GetInputManager();
        if (inp)
        {
            inp.AddActionListener("MenuBack",            EActionTrigger.DOWN, Action_Escape);
            inp.AddActionListener("LobbyGameForceStart", EActionTrigger.DOWN, Action_ForceStart);
            inp.AddActionListener("EditorToggle",        EActionTrigger.DOWN, Action_EditorToggle);
            
            // Наши целевые кнопки
            inp.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
            inp.AddActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
            inp.AddActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNOn);
            inp.AddActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNOff);
            
            // ТЕСТОВАЯ КНОПКА (Пробел). Если она сработает в логах, а T - нет, значит проблема в биндах T.
            inp.AddActionListener("Jump", EActionTrigger.DOWN, Action_TestJump);
            
            Print("[LOBBY DEBUG] Input Listeners registered SUCCESSFULLY!", LogLevel.WARNING);
        }
        else
        {
            Print("[LOBBY DEBUG] ERROR: InputManager is NULL!", LogLevel.ERROR);
        }

        GetGame().GetCallqueue().CallLater(OnTick, UI_TICK_MS, true);
        Print("[LOBBY DEBUG] === OnMenuOpen FINISHED ===", LogLevel.WARNING);
        Print("[LOBBY DEBUG] =====================================================================", LogLevel.WARNING);
    }

    override void OnMenuClose()
    {
        Print("[LOBBY DEBUG] OnMenuClose called!", LogLevel.WARNING);
        
        GetGame().GetCallqueue().Remove(OnTick);
        GetGame().GetCallqueue().Remove(CloseSelf);

        InputManager inp = GetGame().GetInputManager();
        if (inp)
        {
            inp.RemoveActionListener("MenuBack",            EActionTrigger.DOWN, Action_Escape);
            inp.RemoveActionListener("LobbyGameForceStart", EActionTrigger.DOWN, Action_ForceStart);
            inp.RemoveActionListener("EditorToggle",        EActionTrigger.DOWN, Action_EditorToggle);
            
            inp.RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_LobbyVoNOn);
            inp.RemoveActionListener("VONDirect", EActionTrigger.UP, Action_LobbyVoNOff);
            inp.RemoveActionListener("VONChannel", EActionTrigger.DOWN, Action_LobbyVoNOn);
            inp.RemoveActionListener("VONChannel", EActionTrigger.UP, Action_LobbyVoNOff);
            inp.RemoveActionListener("Jump", EActionTrigger.DOWN, Action_TestJump);
        }

        if (m_wChatHudRoot)
            m_wChatHudRoot.SetZOrder(m_iChatHudOriginalZOrder);

        m_aPlayerRows.Clear();
        m_mNames.Clear();
        m_mSquads.Clear();
        m_mRoles.Clear();

        m_wHoveredRoleFrame = null;
        m_bLocalIsTalking = false; 
        s_Instance = null;
    }

    override void OnMenuUpdate(float tDelta) {} 

    override void OnMenuFocusGained()
    {
        Print("[LOBBY DEBUG] Menu Focus GAINED!", LogLevel.WARNING);
        super.OnMenuFocusGained();
    }

    override void OnMenuFocusLost()
    {
        Print("[LOBBY DEBUG] Menu Focus LOST!", LogLevel.WARNING);
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
                        occupiedByW.SetColor(new Color(0.4, 1.0, 0.4, 1.0));
                    else
                        occupiedByW.SetColor(new Color(0.6, 0.6, 0.6, 1.0));
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
                        btn.SetColor(new Color(0.1, 0.55, 0.1, 0.5));
                    else if (isHovered)
                        btn.SetColor(new Color(1.0, 0.75, 0.1, 0.5));
                    else if (isFull)
                        btn.SetColor(new Color(0.35, 0.04, 0.04, 0.5));
                    else
                        btn.SetColor(new Color(0.65, 0.05, 0.05, 0.4));
                }
            }
        }
    }

    protected void RefreshPlayerList()
    {
        if (!m_wPlayerVBox || m_aPlayerRows.IsEmpty()) return;
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
            if (rowIdx >= m_aPlayerRows.Count()) break;
            int psq1 = -1, prl1 = -1;
            if (m_mSquads.Contains(pid1)) psq1 = m_mSquads.Get(pid1);
            if (m_mRoles.Contains(pid1))  prl1 = m_mRoles.Get(pid1);
            string sqN = "--", rlN = "--";
            if (mgr) 
            { 
                sqN = mgr.GetSquadName(psq1); 
                rlN = mgr.GetRoleName(psq1, prl1); 
            }
            Widget row1 = m_aPlayerRows[rowIdx]; 
            row1.SetVisible(true); 
            SetPlayerRow(row1, m_mNames.Get(pid1), sqN, rlN, pid1 == localId, true, pid1); 
            rowIdx++;
        }
        foreach (int pid2 : withoutRole)
        {
            if (rowIdx >= m_aPlayerRows.Count()) break;
            Widget row2 = m_aPlayerRows[rowIdx]; 
            row2.SetVisible(true); 
            SetPlayerRow(row2, m_mNames.Get(pid2), "--", "--", pid2 == localId, false, pid2); 
            rowIdx++;
        }
        for (int i = rowIdx; i < m_aPlayerRows.Count(); i++) 
            m_aPlayerRows[i].SetVisible(false);
    }

    protected void SetPlayerRow(Widget row, string playerName, string squadName, string roleName, bool isMe, bool hasRole, int playerId)
    {
        bool isTalking;
        if (isMe) {
            isTalking = m_bLocalIsTalking;
        } else {
            isTalking = SCR_VonDisplay.IsPlayerTalking(playerId);
        }

        Widget vonIndicator = row.FindAnyWidget("VONIndicator");
        
        if (vonIndicator)
        {
            if (isTalking)
                vonIndicator.SetColor(new Color(0.2, 1.0, 0.2, 1.0)); 
            else
                vonIndicator.SetColor(new Color(0.15, 0.15, 0.15, 1.0)); 
        }

        Color nameColor;
        if (isMe)
            nameColor = new Color(0.4, 0.8, 1.0, 1.0); 
        else if (isTalking)
            nameColor = new Color(0.2, 1.0, 0.2, 1.0); 
        else if (hasRole)
            nameColor = new Color(1.0, 1.0, 1.0, 0.9); 
        else
            nameColor = new Color(0.6, 0.6, 0.6, 1.0);  

        Color infoColor;
        if (hasRole)
            infoColor = new Color(0.85, 0.85, 0.85, 1.0);
        else
            infoColor = new Color(0.5, 0.5, 0.5, 1.0);

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
            if (overlay) overlay.SetColor(new Color(1, 1, 1, 0.08)); 
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
            if (overlay) overlay.SetColor(new Color(1, 1, 1, 0.08));
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

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w == m_wStartButton) 
        { 
            Action_ForceStart(); 
            return true; 
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
    // ПЕРЕХВАТ PTT И ТЕСТЫ
    // =====================================================================
    
    // ТЕСТОВАЯ ФУНКЦИЯ ДЛЯ ПРОБЕЛА
    void Action_TestJump()
    {
        Print("[LOBBY DEBUG] &&&&&&&& TEST JUMP WORKS! Input is reaching LobbyMenu! &&&&&&&&", LogLevel.WARNING);
    }

    void Action_LobbyVoNOn()
    {
        Print("[LOBBY DEBUG] >>> VON ON Action Triggered! >>>", LogLevel.WARNING);
        
        SCR_PlayerController scrPc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
        if (scrPc) 
        {
            Print("[LOBBY DEBUG] SCR_PlayerController found! Calling LobbyVoNEnable...", LogLevel.WARNING);
            scrPc.LobbyVoNEnable();
            m_bLocalIsTalking = true; 
        } 
        else 
        {
            Print("[LOBBY DEBUG] ERROR: SCR_PlayerController is NULL!", LogLevel.ERROR);
        }
    }
    
    void Action_LobbyVoNOff()
    {
        Print("[LOBBY DEBUG] >>> VON OFF Action Triggered! >>>", LogLevel.WARNING);
        
        SCR_PlayerController scrPc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
        if (scrPc) 
        {
            scrPc.LobbyVoNDisable();
        }
        
        m_bLocalIsTalking = false; 
    }
    // =====================================================================

    protected void Action_Escape()     { GetGame().GetCallqueue().CallLater(DoPauseMenu, 0); }
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

    protected void RaiseChatHud()
    {
        WorkspaceWidget ws = GetGame().GetWorkspace();
        if (!ws) return;
        Widget chatPanel = ws.FindAnyWidget("ChatPanel");
        if (!chatPanel) 
        { 
            GetGame().GetCallqueue().CallLater(RaiseChatHud, 100, false); 
            return; 
        }
        Widget w = chatPanel;
        while (w && w.GetParent() != ws) w = w.GetParent();
        if (!w) return;
        m_wChatHudRoot = w;
        m_iChatHudOriginalZOrder = m_wChatHudRoot.GetZOrder();
        m_wChatHudRoot.SetZOrder(1000);
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