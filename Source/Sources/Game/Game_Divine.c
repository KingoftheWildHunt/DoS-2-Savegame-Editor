
//<<>-<>>---------------------------------------------------------------------()
/*
	Gestion des archives du jeu
									      */
//()-------------------------------------------------------------------<<>-<>>//

// �������������������������������������������������������������������������� //
// ���									  ��� //
// ��� Donn�es								  ��� //
// ���									  ��� //
// �������������������������������������������������������������������������� //

#include "Application.h"
#include "Locale.h"
#include "Texts.h"
#include "Divine.h"
#include "Game.h"
#include "LastFiles.h"
#include "Dialogs.h"
#include "Utils.h"
#include "Requests.h"

extern APPLICATION	App;

static DIVINELOGHEADER	LogHeaders[] = {
					{"[DEBUG]", FALSE, IDI_INFORMATION},
					{"[TRACE]", FALSE, IDI_INFORMATION},
					{"[INFO]",  FALSE, IDI_INFORMATION},
					{"[FATAL]",  TRUE, IDI_ERROR},
					{"[ERROR]",  TRUE, IDI_ERROR},
					{NULL,0}
				};

static GAMEEDITPAGE	SaveGamePages[] = {
					{ GAME_PAGE_SAVEGAME_PROFILE, TEXT_SAVEGAME_TITLE_PROFILE, TEXT_SAVEGAME_INFO_PROFILE, 8000 },
					{ GAME_PAGE_SAVEGAME_LIST, TEXT_SAVEGAME_TITLE_LIST, TEXT_SAVEGAME_INFO_LIST, 8001 },
					{ 0 }
				};


// �������������������������������������������������������������������������� //
// ���									  ��� //
// ��� S�lection de la sauvegarde					  ��� //
// ���									  ��� //
// �������������������������������������������������������������������������� //

void Divine_Select()
{
	DIVINESGCONTEXT*	pContext;
	PROPSHEETHEADER*	psh;
	PROPSHEETPAGE*		psp;
	int			iNumPages;
	int			iResult;
	int			i;

	psh = NULL;
	psp = NULL;
	iResult = -1;

	//--- Alloue les structures ---

	pContext = HeapAlloc(App.hHeap,HEAP_ZERO_MEMORY,sizeof(DIVINESGCONTEXT));
	if (!pContext)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto Done;
		}

	psh = HeapAlloc(App.hHeap,HEAP_ZERO_MEMORY,sizeof(PROPSHEETHEADER));
	if (!psh)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto Done;
		}

	for (i = 0, iNumPages = 0; SaveGamePages[i].uPageID != 0; i++, iNumPages++);

	psp = HeapAlloc(App.hHeap,HEAP_ZERO_MEMORY,sizeof(PROPSHEETPAGE)*iNumPages);
	if (!psp)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto Done;
		}

	//--- Valeurs par d�faut ---

	if (App.Config.uGame) pContext->uGame = App.Config.uGame;
	else pContext->uGame = DIVINE_DOS_2EE;

	pContext->pszProfile = Misc_StrCpyAlloc(App.Config.pszProfile);
	pContext->pszSaveName = Misc_StrCpyAlloc(App.Game.Save.pszSaveName);

	//--- Alloue les pages ---

	for (i = 0; SaveGamePages[i].uPageID != 0; i++)
		{
		GAMEEDITPAGECONTEXT*	ctx;

		ctx = HeapAlloc(App.hHeap,HEAP_ZERO_MEMORY,sizeof(GAMEEDITPAGECONTEXT));
		if (!ctx)
			{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			goto Done;
			}

		ctx->savegame.pContext = pContext;
		ctx->uPageID = SaveGamePages[i].uPageID;
		ctx->pszInfo = Locale_GetText(SaveGamePages[i].uInfoID);

		psp[i].dwSize = sizeof(PROPSHEETPAGE);
		psp[i].dwFlags = PSP_USETITLE;
		psp[i].hInstance = App.hInstance;
		psp[i].pszTemplate = MAKEINTRESOURCE(SaveGamePages[i].uResID);
		psp[i].pszTitle = Locale_GetText(SaveGamePages[i].uTitleID);
		psp[i].pfnDlgProc = (DLGPROC)Divine_SelectProc;
		psp[i].lParam = (LPARAM)ctx;
		}

	psh->dwSize = sizeof(PROPSHEETHEADER);
	psh->dwFlags = PSH_PROPSHEETPAGE|PSH_USEICONID|PSH_NOAPPLYNOW|PSH_WIZARD;
	psh->hwndParent = App.hWnd;
	psh->hInstance = App.hInstance;
	psh->pszIcon = MAKEINTRESOURCE(1);
	psh->nPages = iNumPages;
	psh->ppsp = psp;

	iResult = PropertySheet(psh);
	if (iResult) Divine_Open(pContext->uGame,pContext->pszProfile,pContext->pszSaveName);

	//--- Termin� ---

Done:	if (psh) HeapFree(App.hHeap,0,psh);
	if (psp)
		{
		for (i = 0; SaveGamePages[i].uPageID != 0; i++) if (psp[i].lParam) HeapFree(App.hHeap,0,(void *)psp[i].lParam);
		HeapFree(App.hHeap,0,psp);
		}
	if (pContext)
		{
		if (pContext->pszProfile) HeapFree(App.hHeap,0,pContext->pszProfile);
		if (pContext->pszSaveName) HeapFree(App.hHeap,0,pContext->pszSaveName);
		Divine_SelectReleaseList(&pContext->Profiles);
		Divine_SelectReleaseList(&pContext->Savegames);
		HeapFree(App.hHeap,0,pContext);
		}

	if (iResult == -1) Request_PrintError(App.hWnd,Locale_GetText(TEXT_ERR_DIALOG),NULL,MB_ICONERROR);
	return;
}


// ���� Bo�te de dialogue ������������������������������������������������

BOOL CALLBACK Divine_SelectProc(HWND hDlg, UINT uMsgId, WPARAM wParam, LPARAM lParam)
{
	PROPSHEETPAGE*	psp;

	if (uMsgId == WM_MEASUREITEM)
		{
		if (Dialog_ViewComboMeasureItem(190,(MEASUREITEMSTRUCT *)lParam)) return(TRUE);
		((MEASUREITEMSTRUCT *)lParam)->itemWidth = 0;
		((MEASUREITEMSTRUCT *)lParam)->itemHeight = App.Font.uFontHeight+4;
		if (((MEASUREITEMSTRUCT *)lParam)->itemHeight < 24+4) ((MEASUREITEMSTRUCT *)lParam)->itemHeight = 24+4;
		return(TRUE);
		}

	if (uMsgId == WM_INITDIALOG)
		{
		GAMEEDITPAGECONTEXT*	ctx;
		RECT			rcDialog;
		int			Height;

		ctx = (GAMEEDITPAGECONTEXT *)((PROPSHEETPAGE *)lParam)->lParam;
		Height = Dialog_GetInfoHeight(hDlg,100,ctx->pszInfo);
		GetWindowRect(hDlg,&rcDialog);
		// PropSheet dialogs need more work to be resized...
		//SetWindowPos(hDlg,NULL,0,0,rcDialog.right-rcDialog.left,rcDialog.bottom-rcDialog.top+Height,SWP_NOZORDER|SWP_NOMOVE);
		GetWindowRect(GetDlgItem(hDlg,100),&rcDialog);
		SetWindowPos(GetDlgItem(hDlg,100),NULL,0,0,rcDialog.right-rcDialog.left,Height,SWP_NOZORDER|SWP_NOMOVE);
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)lParam);
		SendMessage(hDlg,PSM_SETWIZBUTTONS,(WPARAM)PSWIZB_NEXT,0);

		switch(ctx->uPageID)
			{
			case GAME_PAGE_SAVEGAME_PROFILE:
				Dialog_OffsetY(hDlg,190,Height);
				Dialog_OffsetY(hDlg,200,Height);
				GetWindowRect(GetDlgItem(hDlg,200),&rcDialog);
				MapWindowPoints(NULL,hDlg,(POINT *)&rcDialog,2);
				rcDialog.bottom -= Height;
				SetWindowPos(GetDlgItem(hDlg,200),NULL,0,0,rcDialog.right-rcDialog.left,rcDialog.bottom-rcDialog.top,SWP_NOZORDER|SWP_NOMOVE);
				break;

			case GAME_PAGE_SAVEGAME_LIST:
				Dialog_OffsetY(hDlg,200,Height);
				GetWindowRect(GetDlgItem(hDlg,200),&rcDialog);
				MapWindowPoints(NULL,hDlg,(POINT *)&rcDialog,2);
				rcDialog.bottom -= Height;
				SetWindowPos(GetDlgItem(hDlg,200),NULL,0,0,rcDialog.right-rcDialog.left,rcDialog.bottom-rcDialog.top,SWP_NOZORDER|SWP_NOMOVE);
				break;
			}

		return(TRUE);
		}

	psp = (PROPSHEETPAGE *)GetWindowLongPtr(hDlg,DWLP_USER);
	if (!psp) return(FALSE);

	switch(uMsgId)
		{
		case WM_DRAWITEM:
			switch(wParam)
				{
				case 100:
					Dialog_DrawInfo(((GAMEEDITPAGECONTEXT *)psp->lParam)->pszInfo,(DRAWITEMSTRUCT *)lParam,BF_RECT);
					return(TRUE);
				case 190:
					Divine_SelectDrawGame((DRAWITEMSTRUCT *)lParam);
					return(TRUE);
				case 200:
					Divine_SelectDrawItem((DRAWITEMSTRUCT *)lParam);
					return(TRUE);
				}
			break;

		case WM_COMMAND:
			switch(HIWORD(wParam))
				{
				case LBN_DBLCLK:
					switch(((GAMEEDITPAGECONTEXT *)psp->lParam)->uPageID)
						{
						case GAME_PAGE_SAVEGAME_PROFILE:
							SendMessage(GetParent(hDlg),PSM_PRESSBUTTON,(WPARAM)PSBTN_NEXT,0);
							return(TRUE);
						case GAME_PAGE_SAVEGAME_LIST:
							SendMessage(GetParent(hDlg),PSM_PRESSBUTTON,(WPARAM)PSBTN_FINISH,0);
							return(TRUE);
						}
					break;

				case LBN_SELCHANGE:
					switch(((GAMEEDITPAGECONTEXT *)psp->lParam)->uPageID)
						{
						case GAME_PAGE_SAVEGAME_PROFILE:
							switch(LOWORD(wParam))
								{
								case 190:
									Divine_SelectReleaseList(&((GAMEEDITPAGECONTEXT *)psp->lParam)->savegame.pContext->Profiles);
									Divine_SelectReleaseList(&((GAMEEDITPAGECONTEXT *)psp->lParam)->savegame.pContext->Savegames);
									Divine_SelectCreateList(hDlg,DIVINE_PROFILE_LIST,((GAMEEDITPAGECONTEXT *)psp->lParam)->savegame.pContext);
									return(TRUE);
								case 200:
									PostMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0,(LPARAM)Divine_SelectSetNextPage(hDlg,FALSE,(GAMEEDITPAGECONTEXT *)psp->lParam) != -1?PSWIZB_NEXT:0);
									return(TRUE);
								}
							break;
						case GAME_PAGE_SAVEGAME_LIST:
							switch(LOWORD(wParam))
								{
								case 200:
									PostMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0,(LPARAM)PSWIZB_BACK|(Divine_SelectSetNextPage(hDlg,FALSE,(GAMEEDITPAGECONTEXT *)psp->lParam) != -1?PSWIZB_FINISH:PSWIZB_DISABLEDFINISH));
									return(TRUE);
								}
							break;
						}
					break;
				}
			break;

		case WM_NOTIFY:
			switch(((NMHDR *)lParam)->code)
				{
				case PSN_SETACTIVE:
					SetWindowLongPtr(hDlg,DWLP_MSGRESULT,Divine_SelectActivate(hDlg,(GAMEEDITPAGECONTEXT *)psp->lParam)?0:-1);
					return(TRUE);
				case PSN_WIZBACK:
					SetWindowLongPtr(hDlg,DWLP_MSGRESULT,Divine_SelectSetPrevPage(hDlg,(GAMEEDITPAGECONTEXT *)psp->lParam));
					return(TRUE);
				case PSN_WIZNEXT:
					SetWindowLongPtr(hDlg,DWLP_MSGRESULT,Divine_SelectSetNextPage(hDlg,TRUE,(GAMEEDITPAGECONTEXT *)psp->lParam));
					return(TRUE);
				case PSN_WIZFINISH:
					SetWindowLongPtr(hDlg,DWLP_MSGRESULT,(Divine_SelectSetNextPage(hDlg,TRUE,(GAMEEDITPAGECONTEXT *)psp->lParam) == -1)?TRUE:FALSE);
					return(TRUE);
				}
			break;
		}

	return(FALSE);
}


// ���� Navigation �������������������������������������������������������

//--- Activation d'une page ---

int Divine_SelectActivate(HWND hDlg, GAMEEDITPAGECONTEXT *ctx)
{
	switch(ctx->uPageID)
		{
		case GAME_PAGE_SAVEGAME_PROFILE:
			if (!ctx->bPageSet)
				{
				LRESULT	lResult;

				lResult = SendDlgItemMessage(hDlg,190,CB_ADDSTRING,0,(LPARAM)DIVINE_DOS_2);
				if (lResult == CB_ERR || lResult == CB_ERRSPACE) return(0);
				lResult = SendDlgItemMessage(hDlg,190,CB_ADDSTRING,0,(LPARAM)DIVINE_DOS_2EE);
				if (lResult == CB_ERR || lResult == CB_ERRSPACE) return(0);
				SendDlgItemMessage(hDlg,190,CB_SETCURSEL,(WPARAM)ctx->savegame.pContext->uGame == DIVINE_DOS_2?0:1,0);
				ctx->bPageSet = TRUE;
				}
			if (!Divine_SelectCreateList(hDlg,DIVINE_PROFILE_LIST,ctx->savegame.pContext)) return(0);
			PostMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0,(LPARAM)Divine_SelectSetNextPage(hDlg,FALSE,ctx) != -1?PSWIZB_NEXT:0);
			break;

		case GAME_PAGE_SAVEGAME_LIST:
			if (!ctx->bPageSet)
				{
				ctx->bPageSet = TRUE;
				}
			if (!Divine_SelectCreateList(hDlg,DIVINE_SAVEGAMES_LIST,ctx->savegame.pContext)) return(0);
			PostMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0,(LPARAM)PSWIZB_BACK|(Divine_SelectSetNextPage(hDlg,FALSE,ctx) != -1?PSWIZB_FINISH:PSWIZB_DISABLEDFINISH));
			break;
		}

	return(1);
}

//--- Activation de la page pr�c�dente ---

LONG_PTR Divine_SelectSetPrevPage(HWND hDlg, GAMEEDITPAGECONTEXT *ctx)
{
	switch(ctx->uPageID)
		{
		case GAME_PAGE_SAVEGAME_PROFILE:
			break;

		case GAME_PAGE_SAVEGAME_LIST:
			return((LONG_PTR)SaveGamePages[0].uResID);
		}

	return(-1);
}

//--- Activation de la page suivante ---

LONG_PTR Divine_SelectSetNextPage(HWND hDlg, BOOL bStore, GAMEEDITPAGECONTEXT *ctx)
{
	DIVINEITEM*	pItem;
	UINT		uSelected;
	UINT		uGame;

	switch(ctx->uPageID)
		{
		case GAME_PAGE_SAVEGAME_PROFILE:
			uGame = SendDlgItemMessage(hDlg,190,CB_GETCURSEL,0,0);
			if (uGame == LB_ERR) break;
			uSelected = SendDlgItemMessage(hDlg,200,LB_GETCURSEL,0,0);
			if (uSelected == LB_ERR) break;
			pItem = (DIVINEITEM *)SendDlgItemMessage(hDlg,200,LB_GETITEMDATA,(WPARAM)uSelected,0);
			if (pItem == (DIVINEITEM *)LB_ERR) break;
			if (!pItem) break;
			if (bStore)
				{
				if (ctx->savegame.pContext->pszProfile) HeapFree(App.hHeap,0,ctx->savegame.pContext->pszProfile);
				ctx->savegame.pContext->pszProfile = HeapAlloc(App.hHeap,0,wcslen(pItem->name)*sizeof(WCHAR)+sizeof(WCHAR));
				if (!ctx->savegame.pContext->pszProfile)
					{
					SetLastError(ERROR_NOT_ENOUGH_MEMORY);
					Request_PrintError(hDlg,Locale_GetText(TEXT_ERR_DIALOG),NULL,MB_ICONERROR);
					break;
					}
				wcscpy(ctx->savegame.pContext->pszProfile,pItem->name);
				ctx->savegame.pContext->uGame = ++uGame;
				}
			return(0);

		case GAME_PAGE_SAVEGAME_LIST:
			uSelected = SendDlgItemMessage(hDlg,200,LB_GETCURSEL,0,0);
			if (uSelected == LB_ERR) break;
			pItem = (DIVINEITEM *)SendDlgItemMessage(hDlg,200,LB_GETITEMDATA,(WPARAM)uSelected,0);
			if (pItem == (DIVINEITEM *)LB_ERR) break;
			if (!pItem) break;
			if (bStore)
				{
				if (ctx->savegame.pContext->pszSaveName) HeapFree(App.hHeap,0,ctx->savegame.pContext->pszSaveName);
				ctx->savegame.pContext->pszSaveName = HeapAlloc(App.hHeap,0,wcslen(pItem->name)*sizeof(WCHAR)+sizeof(WCHAR));
				if (!ctx->savegame.pContext->pszSaveName)
					{
					SetLastError(ERROR_NOT_ENOUGH_MEMORY);
					Request_PrintError(hDlg,Locale_GetText(TEXT_ERR_DIALOG),NULL,MB_ICONERROR);
					break;
					}
				wcscpy(ctx->savegame.pContext->pszSaveName,pItem->name);
				}
			return(0);
		}

	return(-1);
}


// ���� Affichage du jeu �������������������������������������������������

void Divine_SelectDrawGame(DRAWITEMSTRUCT *pDraw)
{
	WCHAR*	pszText;

	FillRect(pDraw->hDC,&pDraw->rcItem,GetSysColorBrush(pDraw->itemState&ODS_SELECTED?COLOR_HIGHLIGHT:COLOR_WINDOW));

	pszText = NULL;
	switch(pDraw->itemData)
		{
		case DIVINE_DOS_2:
			pszText = szGameName;
			break;
		case DIVINE_DOS_2EE:
			pszText = szGameNameEE;
			break;
		}

	if (pszText)
		{
		RECT		rcText;
		HFONT		hFont;
		COLORREF	crText;
		int		iBkMode;

		hFont = SelectObject(pDraw->hDC,App.Font.hFont);
		crText = SetTextColor(pDraw->hDC,GetSysColor(pDraw->itemState&ODS_SELECTED?COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT));
		iBkMode = SetBkMode(pDraw->hDC,TRANSPARENT);
		CopyRect(&rcText,&pDraw->rcItem);
		rcText.left += 2;
		rcText.right -= 2;
		DrawText(pDraw->hDC,pszText,-1,&rcText,DT_LEFT|DT_END_ELLIPSIS|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER);
		SetBkMode(pDraw->hDC,iBkMode);
		SetTextColor(pDraw->hDC,crText);
		SelectObject(pDraw->hDC,hFont);
		}

	if (pDraw->itemState&ODS_FOCUS) DrawFocusRect(pDraw->hDC,&pDraw->rcItem);
	return;
}


// ���� Affichage d'un r�pertoire ����������������������������������������

void Divine_SelectDrawItem(DRAWITEMSTRUCT *pDraw)
{
	DIVINEITEM*	pItem;

	FillRect(pDraw->hDC,&pDraw->rcItem,GetSysColorBrush(pDraw->itemState&ODS_SELECTED?COLOR_HIGHLIGHT:COLOR_WINDOW));

	pItem = (DIVINEITEM *)pDraw->itemData;
	if (pItem != 0 && pItem != (DIVINEITEM *)-1)
		{
		RECT		rcText;
		HFONT		hFont;
		COLORREF	crText;
		int		iBkMode;

		hFont = SelectObject(pDraw->hDC,App.Font.hFont);
		crText = SetTextColor(pDraw->hDC,GetSysColor(pDraw->itemState&ODS_SELECTED?COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT));
		iBkMode = SetBkMode(pDraw->hDC,TRANSPARENT);
		CopyRect(&rcText,&pDraw->rcItem);
		rcText.left += 2;
		rcText.right -= 2;
		DrawIconEx(pDraw->hDC,rcText.left,rcText.top+(rcText.bottom-rcText.top-16)/2,App.hIcons[pItem->node.type == DIVINE_PROFILE_LIST?APP_ICON_FOLDER:APP_ICON_SAVEGAME],16,16,0,NULL,DI_NORMAL);
		rcText.left += 20;
		DrawText(pDraw->hDC,pItem->name,-1,&rcText,DT_LEFT|DT_END_ELLIPSIS|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER);
		SetBkMode(pDraw->hDC,iBkMode);
		SetTextColor(pDraw->hDC,crText);
		SelectObject(pDraw->hDC,hFont);
		}

	if (pDraw->itemState&ODS_FOCUS) DrawFocusRect(pDraw->hDC,&pDraw->rcItem);
	return;
}


// ���� Cr�ation d'une liste ���������������������������������������������

int Divine_SelectCreateList(HWND hDlg, UINT uType, DIVINESGCONTEXT *pContext)
{
	WIN32_FIND_DATA		Find;
	HANDLE			hFile;
	DIVINEITEM*		pItem;
	WCHAR*			pszPath;
	WCHAR*			pszSearch;
	NODE*			pRoot;
	UINT			uGame;
	UINT			uLen;
	LRESULT			lResult;
	int			iResult;

	//--- S�lection du r�pertoire ---

	switch(uType)
		{
		case DIVINE_PROFILE_LIST:
			uGame = SendDlgItemMessage(hDlg,190,CB_GETCURSEL,0,0);
			if (uGame == CB_ERR) uGame = 0;
			if (pContext->Profiles.type == ++uGame) return(1);
			SendDlgItemMessage(hDlg,200,LB_RESETCONTENT,0,0);
			uLen  = wcslen(App.Config.pszLarianPath)*sizeof(WCHAR);
			uLen += sizeof(WCHAR)+wcslen(Divine_GetGameName(uGame))*sizeof(WCHAR);
			uLen += sizeof(WCHAR)+wcslen(szPlayerProfiles)*sizeof(WCHAR);
			pszPath = HeapAlloc(App.hHeap,0,uLen+sizeof(WCHAR));
			if (!pszPath)
				{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				Request_PrintError(hDlg,Locale_GetText(TEXT_ERR_DIALOG),NULL,MB_ICONERROR);
				return(0);
				}
			wcscpy(pszPath,App.Config.pszLarianPath);
			PathAppend(pszPath,Divine_GetGameName(uGame));
			PathAppend(pszPath,szPlayerProfiles);
			pRoot = &pContext->Profiles;
			break;

		case DIVINE_SAVEGAMES_LIST:
			if (pContext->Savegames.type == pContext->uGame) return(1);
			SendDlgItemMessage(hDlg,200,LB_RESETCONTENT,0,0);
			uGame = pContext->uGame;
			uLen  = wcslen(App.Config.pszLarianPath)*sizeof(WCHAR);
			uLen += sizeof(WCHAR)+wcslen(Divine_GetGameName(uGame))*sizeof(WCHAR);
			uLen += sizeof(WCHAR)+wcslen(szPlayerProfiles)*sizeof(WCHAR);
			uLen += sizeof(WCHAR)+wcslen(szSavegames)*sizeof(WCHAR);
			uLen += sizeof(WCHAR)+wcslen(szStory)*sizeof(WCHAR);
			uLen += sizeof(WCHAR)+wcslen(pContext->pszProfile)*sizeof(WCHAR);
			pszPath = HeapAlloc(App.hHeap,0,uLen+sizeof(WCHAR));
			if (!pszPath)
				{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				Request_PrintError(hDlg,Locale_GetText(TEXT_ERR_DIALOG),NULL,MB_ICONERROR);
				return(0);
				}
			wcscpy(pszPath,App.Config.pszLarianPath);
			PathAppend(pszPath,Divine_GetGameName(uGame));
			PathAppend(pszPath,szPlayerProfiles);
			PathAppend(pszPath,pContext->pszProfile);
			PathAppend(pszPath,szSavegames);
			PathAppend(pszPath,szStory);
			pRoot = &pContext->Savegames;
			break;

		default:return(0);
		}

	Divine_SelectReleaseList(pRoot);

	//--- V�rifie que le r�pertoire existe ---

	if (!PathFileExists(pszPath))
		{
		HeapFree(App.hHeap,0,pszPath);
		return(1);
		}

	//--- R�cup�re la liste des dossiers dans le r�pertoire ---

	pszSearch = HeapAlloc(App.hHeap,0,wcslen(pszPath)*sizeof(WCHAR)+sizeof(WCHAR)+wcslen(szWild)*sizeof(WCHAR)+sizeof(WCHAR));
	if (!pszSearch)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		Request_PrintError(hDlg,Locale_GetText(TEXT_ERR_DIALOG),NULL,MB_ICONERROR);
		HeapFree(App.hHeap,0,pszPath);
		return(0);
		}
	wcscpy(pszSearch,pszPath);
	PathAppend(pszSearch,szWild);
	hFile = FindFirstFile(pszSearch,&Find);
	HeapFree(App.hHeap,0,pszSearch);
	if (hFile == INVALID_HANDLE_VALUE)
		{
		Request_PrintError(hDlg,Locale_GetText(TEXT_ERR_DIALOG),NULL,MB_ICONERROR);
		HeapFree(App.hHeap,0,pszPath);
		return(0);
		}

	iResult = 0;

	do {

		if (!wcscmp(Find.cFileName,szRootPath)) continue;
		if (!wcscmp(Find.cFileName,szParentPath)) continue;

		if (Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
			//--- V�rifie que le profil contient bien le r�pertoire des sauvegardes
			if (uType == DIVINE_PROFILE_LIST)
				{
				WCHAR*	pszTemp;
				UINT	uLen;

				uLen  = wcslen(pszPath);
				uLen += 1+wcslen(Find.cFileName);
				uLen += 1+wcslen(szSavegames);
				uLen += 1+wcslen(szStory);
				pszTemp = HeapAlloc(App.hHeap,0,uLen*sizeof(WCHAR)+sizeof(WCHAR));
				if (!pszTemp)
					{
					SetLastError(ERROR_NOT_ENOUGH_MEMORY);
					Request_PrintError(hDlg,Locale_GetText(TEXT_ERR_DIALOG),NULL,MB_ICONERROR);
					goto Done;
					}
				wcscpy(pszTemp,pszPath);
				PathAppend(pszTemp,Find.cFileName);
				PathAppend(pszTemp,szSavegames);
				PathAppend(pszTemp,szStory);
				if (!PathFileExists(pszTemp))
					{
					HeapFree(App.hHeap,0,pszTemp);
					continue;
					}
				HeapFree(App.hHeap,0,pszTemp);
				}

			//--- Ajoute le dossier � la liste interne
			pItem = HeapAlloc(App.hHeap,0,sizeof(DIVINEITEM));
			if (!pItem)
				{
				Request_PrintError(hDlg,Locale_GetText(TEXT_ERR_DIALOG),NULL,MB_ICONERROR);
				goto Done;
				}
			List_AddEntry((NODE *)pItem,pRoot);
			pItem->node.type = uType;
			pItem->name = Misc_StrCpyAlloc(Find.cFileName);
			if (!pItem->name)
				{
				Request_PrintError(hDlg,Locale_GetText(TEXT_ERR_DIALOG),NULL,MB_ICONERROR);
				goto Done;
				}

			//--- Ajoute le dossier � la liste visuelle
			lResult = SendDlgItemMessage(hDlg,200,LB_ADDSTRING,0,(LPARAM)pItem);
			if (lResult == LB_ERR || lResult == LB_ERRSPACE)
				{
				Request_PrintError(hDlg,Locale_GetText(TEXT_ERR_DIALOG),NULL,MB_ICONERROR);
				goto Done;
				}
			switch(uType)
				{
				case DIVINE_PROFILE_LIST:
					if (!pContext->pszProfile) break;
					if (wcscmp(pItem->name,pContext->pszProfile)) break;
					SendDlgItemMessage(hDlg,200,LB_SETCURSEL,(WPARAM)lResult,0);
					PostMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0,(LPARAM)PSWIZB_NEXT);
					break;
				case DIVINE_SAVEGAMES_LIST:
					if (!pContext->pszSaveName) break;
					if (wcscmp(pItem->name,pContext->pszSaveName)) break;
					SendDlgItemMessage(hDlg,200,LB_SETCURSEL,(WPARAM)lResult,0);
					PostMessage(GetParent(hDlg),PSM_SETWIZBUTTONS,0,(LPARAM)PSWIZB_FINISH);
					break;
				}
			}

	} while (FindNextFile(hFile,&Find));

	iResult = 1;

Done:	FindClose(hFile);
	HeapFree(App.hHeap,0,pszPath);

	if (iResult) pRoot->type = uGame;
	else Divine_SelectReleaseList(pRoot);

	return(iResult);
}


// ���� Lib�ration de la m�moire utilis�e par une liste ������������������

void Divine_SelectReleaseList(NODE *pRoot)
{
	DIVINEITEM*	pItem;

	for (pItem = (DIVINEITEM *)pRoot->next; pItem != NULL; pItem = (DIVINEITEM *)pItem->node.next)
		{
		if (pItem->name) HeapFree(App.hHeap,0,pItem->name);
		}

	pRoot->type = 0;
	List_ReleaseMemory(pRoot);
	return;
}


// �������������������������������������������������������������������������� //
// ���									  ��� //
// ��� Chargement							  ��� //
// ���									  ��� //
// �������������������������������������������������������������������������� //

// ���� Pr�paration du chargement ����������������������������������������

void Divine_Open(UINT uGame, WCHAR *pszProfile, WCHAR *pszSaveName)
{
	DIVINECONTEXT*	ctx;

	if (App.hThread)
		{
		MessageBox(App.hWnd,Locale_GetText(TEXT_ERR_RUNNING),NULL,MB_ICONERROR);
		return;
		}

	if (!PathFileExists(szDivineEXE))
		{
		MessageBox(App.hWnd,Locale_GetText(TEXT_ERR_MISSINGCONVERTER),NULL,MB_ICONERROR);
		return;
		}

	ctx = HeapAlloc(App.hHeap,HEAP_ZERO_MEMORY,sizeof(DIVINECONTEXT));
	if (!ctx)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		Request_PrintError(App.hWnd,Locale_GetText(TEXT_ERR_LOADING),NULL,MB_ICONERROR);
		return;
		}

	ctx->uGame = uGame;

	ctx->pszProfile = Misc_StrCpyAlloc(pszProfile);
	if (!ctx->pszProfile)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		Request_PrintError(App.hWnd,Locale_GetText(TEXT_ERR_LOADING),NULL,MB_ICONERROR);
		Divine_ReleaseContext(ctx);
		return;
		}

	ctx->pszSaveName = Misc_StrCpyAlloc(pszSaveName);
	if (!ctx->pszSaveName)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		Request_PrintError(App.hWnd,Locale_GetText(TEXT_ERR_LOADING),NULL,MB_ICONERROR);
		Divine_ReleaseContext(ctx);
		return;
		}

	App.hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Divine_LoadThread,ctx,0,NULL);
	if (!App.hThread)
		{
		Request_PrintError(App.hWnd,Locale_GetText(TEXT_ERR_LOADING),NULL,MB_ICONERROR);
		Divine_ReleaseContext(ctx);
		return;
		}

	return;
}


// ���� T�che de chargement ����������������������������������������������

DWORD WINAPI Divine_LoadThread(DIVINECONTEXT *ctx)
{
	Divine_Close();
	Game_Lock(GAME_LOCK_DISABLED|GAME_LOCK_APP);

	#if _DEBUG
	ctx->uDebugLevel = DIVINE_DEBUG_ALL;
	#else
	ctx->uDebugLevel = DIVINE_DEBUG_ERROR;
	#endif

	ctx->dwResult = Divine_Execute(DIVINE_EXTRACT,ctx);
	if (ctx->dwResult != ERROR_SUCCESS) goto Done;

	ctx->dwResult = Divine_Execute(DIVINE_CONVERTLSF,ctx);
	if (ctx->dwResult != ERROR_SUCCESS) goto Done;

	ctx->pszPath = Divine_GetTempPath(4,szTempPath,szSavegames,ctx->pszSaveName,szGlobalsLSX);
	if (ctx->pszPath)
		{
		if (xml_LoadFile(ctx->pszPath))
			{
			Game_BuildPlayers();
			Game_Lock(GAME_LOCK_ENABLED|GAME_LOCK_FILE);

			LastFiles_Add(Divine_GetGameName(ctx->uGame),ctx->pszProfile,ctx->pszSaveName);
			Misc_SetWindowText(App.hWnd,&App.pszWindowTitle,szTitle,szTitleFmt,szTitle,ctx->pszSaveName);

			if (App.Config.pszProfile) HeapFree(App.hHeap,0,App.Config.pszProfile);
			App.Config.uGame = ctx->uGame;
			App.Config.pszProfile = ctx->pszProfile;
			App.Game.Save.pszSaveName = ctx->pszSaveName;
			ctx->pszProfile = NULL;
			ctx->pszSaveName = NULL;
			}
		}
	else xml_SendErrorMsg(XML_ERROR_FROM_SYSTEM,0);

Done:	if (ctx->dwResult != ERROR_SUCCESS)
		{
		SetLastError(ctx->dwResult);
		if (!ctx->bNoErrorMsg) xml_SendErrorMsg(XML_ERROR_FROM_SYSTEM,0);
		}

	Status_SetText(Locale_GetText(TEXT_DONE));
	Game_Lock(GAME_LOCK_ENABLED|GAME_LOCK_APP);
	Divine_ReleaseContext(ctx);
	App.hThread = NULL;
	return(0);
}


// �������������������������������������������������������������������������� //
// ���									  ��� //
// ��� Sauvegarde							  ��� //
// ���									  ��� //
// �������������������������������������������������������������������������� //

// ���� Pr�paration de la sauvegarde �������������������������������������

void Divine_Write()
{
	DIVINECONTEXT*	ctx;

	if (!App.Config.pszProfile || !App.Game.Save.pszSaveName) return;

	if (App.hThread)
		{
		MessageBox(App.hWnd,Locale_GetText(TEXT_ERR_RUNNING),NULL,MB_ICONERROR);
		return;
		}

	if (!PathFileExists(szDivineEXE))
		{
		MessageBox(App.hWnd,Locale_GetText(TEXT_ERR_MISSINGCONVERTER),NULL,MB_ICONERROR);
		return;
		}

	ctx = HeapAlloc(App.hHeap,HEAP_ZERO_MEMORY,sizeof(DIVINECONTEXT));
	if (!ctx)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		Request_PrintError(App.hWnd,Locale_GetText(TEXT_ERR_SAVING),NULL,MB_ICONERROR);
		return;
		}

	ctx->uGame = App.Config.uGame;
	ctx->pszProfile = Misc_StrCpyAlloc(App.Config.pszProfile);
	if (!ctx->pszProfile)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		Request_PrintError(App.hWnd,Locale_GetText(TEXT_ERR_SAVING),NULL,MB_ICONERROR);
		Divine_ReleaseContext(ctx);
		return;
		}
	ctx->pszSaveName = Misc_StrCpyAlloc(App.Game.Save.pszSaveName);
	if (!ctx->pszSaveName)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		Request_PrintError(App.hWnd,Locale_GetText(TEXT_ERR_SAVING),NULL,MB_ICONERROR);
		Divine_ReleaseContext(ctx);
		return;
		}

	App.hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Divine_SaveThread,ctx,0,NULL);
	if (!App.hThread)
		{
		Request_PrintError(App.hWnd,Locale_GetText(TEXT_ERR_SAVING),NULL,MB_ICONERROR);
		Divine_ReleaseContext(ctx);
		return;
		}

	return;
}


// ���� T�che de sauvegarde ����������������������������������������������

DWORD WINAPI Divine_SaveThread(DIVINECONTEXT *ctx)
{
	Game_Lock(GAME_LOCK_DISABLED|GAME_LOCK_ALL);

	#if _DEBUG
	ctx->uDebugLevel = DIVINE_DEBUG_ALL;
	#else
	ctx->uDebugLevel = DIVINE_DEBUG_ERROR;
	#endif

	ctx->pszPath = Divine_GetTempPath(4,szTempPath,szSavegames,ctx->pszSaveName,szGlobalsLSX);
	if (ctx->pszPath)
		{
		if (!xml_SaveFile(ctx->pszPath)) goto Done;
		}
	else
		{
		xml_SendErrorMsg(XML_ERROR_FROM_SYSTEM,0);
		goto Done;
		}

	ctx->dwResult = Divine_Execute(DIVINE_CONVERTLSX,ctx);
	if (ctx->dwResult != ERROR_SUCCESS) goto Done;

	ctx->dwResult = Divine_Execute(DIVINE_CREATE,ctx);

Done:	Status_SetText(Locale_GetText(TEXT_DONE));
	Game_Lock(GAME_LOCK_ENABLED|GAME_LOCK_ALL);
	Divine_ReleaseContext(ctx);
	App.hThread = NULL;
	return(0);
}


// �������������������������������������������������������������������������� //
// ���									  ��� //
// ��� Gestion								  ��� //
// ���									  ��� //
// �������������������������������������������������������������������������� //

// ���� Ex�cution d'une op�ration (divine.exe) ���������������������������

DWORD Divine_Execute(UINT uType, DIVINECONTEXT *ctx)
{
	SECURITY_ATTRIBUTES	Security;
	PROCESS_INFORMATION	ProcInfo;
	STARTUPINFO		Startup;
	HANDLE			hChildStdOutRead,hChildStdOutWrite;
	HANDLE			hChildStdInRead,hChildStdInWrite;
	WCHAR*			pszFmt;
	WCHAR*			pszParameters;
	WCHAR*			pszTemp;
	DWORD			dwLastError;
	DWORD			dwRead;
	DWORD_PTR		vl[3];

	dwLastError = ERROR_SUCCESS;
	hChildStdOutRead = hChildStdOutWrite = INVALID_HANDLE_VALUE;
	hChildStdInRead = hChildStdInWrite = INVALID_HANDLE_VALUE;
	vl[1] = vl[2] = 0;

	switch(ctx->uDebugLevel)
		{
		case DIVINE_DEBUG_ALL:
			vl[0] = (DWORD_PTR)L"all";
			break;
		default:vl[0] = (DWORD_PTR)L"error";
			break;
		}

	//--- Pr�paration des param�tres ---

	switch(uType)
		{
		case DIVINE_EXTRACT:
			pszFmt = szDivineExtract;

			vl[1] = (DWORD_PTR)Divine_GetSaveGamePath(ctx->uGame,ctx->pszProfile,ctx->pszSaveName);
			if (!vl[1])
				{
				dwLastError = GetLastError();
				goto Done;
				}

			if (!PathFileExists((WCHAR *)vl[1]))
				{
				dwLastError = GetLastError();
				goto Done;
				}

			vl[2] = (DWORD_PTR)Divine_CreateTempPath(3,szTempPath,szSavegames,ctx->pszSaveName);
			if (!vl[2])
				{
				dwLastError = GetLastError();
				goto Done;
				}

			Status_SetText(Locale_GetText(TEXT_LOADING_EXTRACT),ctx->pszSaveName);
			break;

		case DIVINE_CREATE:
			pszFmt = szDivineCreate;

			vl[1] = (DWORD_PTR)Divine_GetTempPath(3,szTempPath,szSavegames,ctx->pszSaveName);
			if (!vl[1])
				{
				dwLastError = GetLastError();
				goto Done;
				}

			vl[2] = (DWORD_PTR)Divine_GetTempPath(3,szTempPath,szSavegames,ctx->pszSaveName);
			if (!vl[2])
				{
				dwLastError = GetLastError();
				goto Done;
				}
			pszTemp = HeapAlloc(App.hHeap,0,wcslen((WCHAR *)vl[2])*sizeof(WCHAR)+wcslen(szLSVext)*sizeof(WCHAR)+sizeof(WCHAR));
			if (!pszTemp)
				{
				dwLastError = GetLastError();
				goto Done;
				}
			wcscpy(pszTemp,(WCHAR *)vl[2]);
			wcscat(pszTemp,szLSVext);
			HeapFree(App.hHeap,0,(WCHAR *)vl[2]);
			vl[2] = (DWORD_PTR)pszTemp;

			Status_SetText(Locale_GetText(TEXT_SAVING_CREATE),ctx->pszSaveName);
			break;

		case DIVINE_CONVERTLSF:
			pszFmt = szDivineConvertLSF;

			vl[1] = (DWORD_PTR)Divine_GetTempPath(4,szTempPath,szSavegames,ctx->pszSaveName,szGlobalsLSF);
			if (!vl[1])
				{
				dwLastError = GetLastError();
				goto Done;
				}

			vl[2] = (DWORD_PTR)Divine_GetTempPath(4,szTempPath,szSavegames,ctx->pszSaveName,szGlobalsLSX);
			if (!vl[2])
				{
				dwLastError = GetLastError();
				goto Done;
				}

			Status_SetText(Locale_GetText(TEXT_LOADING_CONVERTLSF),ctx->pszSaveName,szGlobalsLSF);
			break;

		case DIVINE_CONVERTLSX:
			pszFmt = szDivineConvertLSX;

			vl[1] = (DWORD_PTR)Divine_GetTempPath(4,szTempPath,szSavegames,ctx->pszSaveName,szGlobalsLSX);
			if (!vl[1])
				{
				dwLastError = GetLastError();
				goto Done;
				}

			vl[2] = (DWORD_PTR)Divine_GetTempPath(4,szTempPath,szSavegames,ctx->pszSaveName,szGlobalsLSF);
			if (!vl[2])
				{
				dwLastError = GetLastError();
				goto Done;
				}

			Status_SetText(Locale_GetText(TEXT_SAVING_CONVERTLSX),ctx->pszSaveName,szGlobalsLSX);
			break;
		}

	if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,pszFmt,0,0,(WCHAR *)&pszParameters,1,(va_list *)&vl))
		{
		dwLastError = GetLastError();
		goto Done;
		}

	//--- Ex�cution de l'op�ration ---

	Security.nLength = sizeof(SECURITY_ATTRIBUTES);
	Security.bInheritHandle = TRUE;
	Security.lpSecurityDescriptor = NULL;
	if (!CreatePipe(&hChildStdOutRead,&hChildStdOutWrite,&Security,0))
		{
		dwLastError = GetLastError();
		goto Done;
		}
	if (!SetHandleInformation(hChildStdOutRead,HANDLE_FLAG_INHERIT,0))
		{
		dwLastError = GetLastError();
		goto Done;
		}
	if (!CreatePipe(&hChildStdInRead,&hChildStdInWrite,&Security,0))
		{
		dwLastError = GetLastError();
		goto Done;
		}
	if (!SetHandleInformation(hChildStdInWrite,HANDLE_FLAG_INHERIT,0))
		{
		dwLastError = GetLastError();
		goto Done;
		}

	ZeroMemory(&Startup,sizeof(STARTUPINFO));
	ZeroMemory(&ProcInfo,sizeof(PROCESS_INFORMATION));
	Startup.cb = sizeof(STARTUPINFO);
	Startup.dwFlags = STARTF_USESTDHANDLES;
	Startup.hStdInput = hChildStdInRead;
	Startup.hStdOutput = hChildStdOutWrite;
	Startup.hStdError = hChildStdOutWrite;
	if (CreateProcess(szDivineEXE,pszParameters,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&Startup,&ProcInfo))
		{
		WaitForSingleObject(ProcInfo.hProcess,INFINITE);
		CloseHandle(ProcInfo.hProcess);
		CloseHandle(ProcInfo.hThread);
		if (PeekNamedPipe(hChildStdOutRead,NULL,0,NULL,&dwRead,NULL) && dwRead)
			{
			ctx->pszLog = HeapAlloc(App.hHeap,HEAP_ZERO_MEMORY,dwRead+1);
			if (!ctx->pszLog)
				{
				dwLastError = ERROR_NOT_ENOUGH_MEMORY;
				goto Done;
				}
			PeekNamedPipe(hChildStdOutRead,ctx->pszLog,dwRead,NULL,NULL,NULL);
			dwLastError = Divine_ParseLog(ctx);
			HeapFree(App.hHeap,0,ctx->pszLog);
			ctx->pszLog = NULL;
			}
		}
	else dwLastError = GetLastError();
	LocalFree(pszParameters);

	//--- Termin� ! ---

Done:	if (dwLastError == ERROR_SUCCESS)
		{
		switch (uType)
			{
			case DIVINE_CONVERTLSX:
				if (!DeleteFile((WCHAR *)vl[1])) dwLastError = GetLastError();
				break;

			case DIVINE_CREATE:
				pszTemp = Divine_GetSaveGamePath(ctx->uGame,ctx->pszProfile,ctx->pszSaveName);
				if (pszTemp)
					{
					if (!MoveFileEx((WCHAR *)vl[2],pszTemp,MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING)) dwLastError = GetLastError();
					HeapFree(App.hHeap,0,pszTemp);
					}
				else dwLastError = GetLastError();
				break;
			}
		}

	if (hChildStdInRead != INVALID_HANDLE_VALUE) CloseHandle(hChildStdInRead);
	if (hChildStdInWrite != INVALID_HANDLE_VALUE) CloseHandle(hChildStdInWrite);
	if (hChildStdOutWrite != INVALID_HANDLE_VALUE) CloseHandle(hChildStdOutWrite);
	if (hChildStdOutRead != INVALID_HANDLE_VALUE) CloseHandle(hChildStdOutRead);
	if (vl[2]) HeapFree(App.hHeap,0,(WCHAR *)vl[2]);
	if (vl[1]) HeapFree(App.hHeap,0,(WCHAR *)vl[1]);
	return(dwLastError);
}


// ���� Lib�ration des donn�es de gestion ��������������������������������

void Divine_ReleaseContext(DIVINECONTEXT *ctx)
{
	if (!ctx) return;
	if (ctx->pszSaveName) HeapFree(App.hHeap,0,ctx->pszSaveName);
	if (ctx->pszProfile) HeapFree(App.hHeap,0,ctx->pszProfile);
	if (ctx->pszPath) HeapFree(App.hHeap,0,ctx->pszPath);
	HeapFree(App.hHeap,0,ctx);
	return;
}


// ���� Fermeture de la sauvegarde ���������������������������������������

void Divine_Close()
{
	Misc_SetWindowText(App.hWnd,&App.pszWindowTitle,szTitle,NULL);

	Game_ReleasePlayers();
	xml_ReleaseAll(&App.Game.Save.nodeXMLRoot);
	Divine_Cleanup();

	if (App.Game.Save.pszSaveName) HeapFree(App.hHeap,0,App.Game.Save.pszSaveName);
	App.Game.Save.pszSaveName = NULL;

	Game_Lock(GAME_LOCK_DISABLED|GAME_LOCK_FILE|GAME_LOCK_TREE);
	return;
}


// ���� Nettoyage du r�pertoire temporaire �������������������������������

void Divine_Cleanup()
{
	WCHAR*	pszPath;

	pszPath = Divine_GetTempPath(1,szTempPath);
	if (!pszPath) return;
	Divine_CleanupLoop(pszPath);
	HeapFree(App.hHeap,0,pszPath);
	return;
}

//--- Boucle de suppression des fichiers ---

int Divine_CleanupLoop(WCHAR *pszPath)
{
	WIN32_FIND_DATA		Find;
	HANDLE			hFile;
	WCHAR*			pszCurrent;
	DWORD			dwLen;

	//--- Copie le r�pertoire actuel
	dwLen = GetCurrentDirectory(0,NULL);
	pszCurrent = HeapAlloc(App.hHeap,0,dwLen*sizeof(WCHAR));
	if (!pszCurrent)
		{
		HeapFree(App.hHeap,0,pszPath);
		return(0);
		}
	GetCurrentDirectory(dwLen,pszCurrent);

	//--- Modifie le r�pertoire
	if (!SetCurrentDirectory(pszPath))
		{
		HeapFree(App.hHeap,0,pszCurrent);
		return(0);
		}

	//--- Retrouve les fichiers dans le r�pertoire
	hFile = FindFirstFile(szWild,&Find);
	if (hFile == INVALID_HANDLE_VALUE)
		{
		HeapFree(App.hHeap,0,pszCurrent);
		return(0);
		}

	do {

		if (!wcscmp(Find.cFileName,szRootPath)) continue;
		if (!wcscmp(Find.cFileName,szParentPath)) continue;

		if (Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
			//--- Parcours le r�pertoire
			if (!Divine_CleanupLoop(Find.cFileName))
				{
				FindClose(hFile);
				HeapFree(App.hHeap,0,pszCurrent);
				return(0);
				}
			//--- Supprime le r�pertoire
			RemoveDirectory(Find.cFileName);
			continue;
			}

		//--- Supprime le fichier
		DeleteFile(Find.cFileName);

	} while (FindNextFile(hFile,&Find));

	FindClose(hFile);

	//--- Restaure le r�pertoire actuel
	if (!SetCurrentDirectory(pszCurrent))
		{
		HeapFree(App.hHeap,0,pszCurrent);
		return(0);
		}

	HeapFree(App.hHeap,0,pszCurrent);
	return(1);
}


// �������������������������������������������������������������������������� //
// ���									  ��� //
// ��� Logs								  ��� //
// ���									  ��� //
// �������������������������������������������������������������������������� //

// ���� Analyse du journal �����������������������������������������������

DWORD Divine_ParseLog(DIVINECONTEXT *ctx)
{
	DIVINELOGHEADER*	pHeader;
	int			i;

	for (i = 0; ctx->pszLog[i] != 0; i++)
		{
		if ((pHeader = Divine_GetHeader(&ctx->pszLog[i])) != NULL)
			{
			if (pHeader->bIsError)
				{
				if (MessageBox(App.hWnd,Locale_GetText(TEXT_ERR_DIVINE),NULL,MB_ICONERROR|MB_YESNO) == IDYES) Divine_ShowLog(ctx);
				ctx->bNoErrorMsg = TRUE;
				return(ERROR_INVALID_DATA);
				}
			}
		while (ctx->pszLog[i] != '\n')
			{
			if (ctx->pszLog[i] == 0)
				{
				i--;
				break;
				}
			i++;
			}
		}

	return(ERROR_SUCCESS);
}


// ���� Retrouve les informations d'une ligne du journal �����������������

DIVINELOGHEADER* Divine_GetHeader(char *pszLogLine)
{
	int	i;

	for (i = 0; LogHeaders[i].pszHeader != NULL; i++)
		{
		if (strncmp(pszLogLine,LogHeaders[i].pszHeader,strlen(LogHeaders[i].pszHeader))) continue;
		return(&LogHeaders[i]);
		}

	return(NULL);
}


// ���� Affichage du journal ���������������������������������������������

void Divine_ShowLog(DIVINECONTEXT *ctx)
{
	DIVINELOG*		pLog;
	DIVINELOGHEADER*	pHeader;
	WCHAR*			pszResIcon;
	char*			pszHeader;
	char*			pszStart;
	char*			pszEnd;
	UINT			uLen;
	UINT			uWideLen;

	pszStart = ctx->pszLog;
	while (*pszStart != 0)
		{
		pszEnd = strchr(pszStart,'\r');
		if (!pszEnd) pszEnd = strchr(pszStart,'\n');
		if (!pszEnd) pszEnd = strchr(pszStart,'\0');
		if (!pszEnd) break;
		pHeader = Divine_GetHeader(pszStart);
		if (pHeader) pszResIcon = pHeader->pszResIcon;
		else pszResIcon = NULL;
		pszHeader = strchr(pszStart,']');
		if (pszHeader) pszStart = ++pszHeader;
		uLen = pszEnd-pszStart;
		uWideLen = MultiByteToWideChar(CP_ACP,0,pszStart,uLen,NULL,0);
		pLog = HeapAlloc(App.hHeap,HEAP_ZERO_MEMORY,sizeof(DIVINELOG)+uWideLen*sizeof(WCHAR)+sizeof(WCHAR));
		if (!pLog) break;
		pLog->icon = pszResIcon;
		MultiByteToWideChar(CP_ACP,0,pszStart,uLen,pLog->line,uWideLen);
		List_AddEntry((NODE *)pLog,&ctx->Log);
		pszStart = pszEnd;
		if (*pszStart == '\r') pszStart++;
		if (*pszStart == 0) break;
		if (*pszStart == '\n') pszStart++;
		}

	DialogBoxParam(App.hInstance,MAKEINTRESOURCE(1008),App.hWnd,Divine_LogProc,(LPARAM)&ctx->Log);
	List_ReleaseMemory(&ctx->Log);
	return;
}


// ���� Messages de la bo�te de dialogue ���������������������������������

INT_PTR CALLBACK Divine_LogProc(HWND hDlg, UINT uMsgId, WPARAM wParam, LPARAM lParam)
{
	if (uMsgId == WM_MEASUREITEM)
		{
		((MEASUREITEMSTRUCT *)lParam)->itemWidth = 0;
		((MEASUREITEMSTRUCT *)lParam)->itemHeight = App.Font.uFontHeight+4;
		if (((MEASUREITEMSTRUCT *)lParam)->itemHeight < DIVINE_ICON_SIZE+4) ((MEASUREITEMSTRUCT *)lParam)->itemHeight = DIVINE_ICON_SIZE+4;
		return(TRUE);
		}

	if (uMsgId == WM_INITDIALOG)
		{
		DIVINELOG*	pLog;
		LRESULT		lResult;

		for (pLog = (DIVINELOG *)((NODE *)lParam)->next; pLog != NULL; pLog = (DIVINELOG *)pLog->node.next)
			{
			lResult = SendDlgItemMessage(hDlg,200,LB_ADDSTRING,0,(LPARAM)pLog);
			if (lResult == LB_ERR || lResult == LB_ERRSPACE)
				{
				EndDialog(hDlg,-1);
				return(FALSE);
				}
			}

		SendDlgItemMessage(hDlg,IDOK,WM_SETTEXT,0,(LPARAM)Locale_GetText(TEXT_OK));
		SetWindowText(hDlg,Locale_GetText(TEXT_DIALOG_TITLE_LOG));
		Dialog_CenterWindow(hDlg,App.hWnd);
		return(FALSE);
		}

	switch(uMsgId)
		{
		case WM_DRAWITEM:
			switch(wParam)
				{
				case 200:
					Divine_DrawLogLine((DRAWITEMSTRUCT *)lParam);
					return(TRUE);
				}
			break;

		case WM_COMMAND:
			switch(HIWORD(wParam))
				{
				case BN_CLICKED:
					switch(LOWORD(wParam))
						{
						case IDOK:
							EndDialog(hDlg,IDOK);
							return(TRUE);
						case IDCANCEL:
							EndDialog(hDlg,IDCANCEL);
							return(TRUE);
						}
					break;
				}
			break;

		case WM_CLOSE:
			EndDialog(hDlg,IDCANCEL);
			return(TRUE);
		}

	return(FALSE);
}

//--- Affichage d'une ligne ---

void Divine_DrawLogLine(DRAWITEMSTRUCT *pDraw)
{
	DIVINELOG*	pLog;
	HFONT		hFont;
	RECT		rcDraw;
	COLORREF	crText;
	int		iBack;

	FillRect(pDraw->hDC,&pDraw->rcItem,GetSysColorBrush((pDraw->itemState&ODS_SELECTED)?COLOR_HIGHLIGHT:COLOR_WINDOW));
	if (pDraw->itemState&ODS_FOCUS) DrawFocusRect(pDraw->hDC,&pDraw->rcItem);

	pLog = (DIVINELOG *)pDraw->itemData;
	if (!pLog) return;
	if (pLog == (DIVINELOG *)-1) return;

	hFont = SelectObject(pDraw->hDC,App.Font.hFont);
	crText = SetTextColor(pDraw->hDC,GetSysColor((pDraw->itemState&ODS_SELECTED)?COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT));
	iBack = SetBkMode(pDraw->hDC,TRANSPARENT);

	CopyRect(&rcDraw,&pDraw->rcItem);
	rcDraw.left += 4;
	rcDraw.right -= 14;
	rcDraw.top += 1;
	rcDraw.bottom -= 1;
	if (pLog->icon) DrawIconEx(pDraw->hDC,rcDraw.left,rcDraw.top+(rcDraw.bottom-rcDraw.top-DIVINE_ICON_SIZE)/2,LoadImage(NULL,MAKEINTRESOURCE(pLog->icon),IMAGE_ICON,DIVINE_ICON_SIZE,DIVINE_ICON_SIZE,LR_DEFAULTCOLOR|LR_SHARED),DIVINE_ICON_SIZE,DIVINE_ICON_SIZE,0,NULL,DI_NORMAL);
	rcDraw.left += DIVINE_ICON_SIZE+4;
	DrawText(pDraw->hDC,pLog->line,-1,&rcDraw,DT_LEFT|DT_NOPREFIX|DT_PATH_ELLIPSIS|DT_SINGLELINE|DT_VCENTER);

	SetBkMode(pDraw->hDC,iBack);
	SetTextColor(pDraw->hDC,crText);
	SelectObject(pDraw->hDC,hFont);
	return;
}


// �������������������������������������������������������������������������� //
// ���									  ��� //
// ��� Sous-routines							  ��� //
// ���									  ��� //
// �������������������������������������������������������������������������� //

// ���� Cr�ation du r�pertoire temporaire ��������������������������������

WCHAR* Divine_CreateTempPath(UINT uNumPaths, ...)
{
	WCHAR*		pszTempPath;
	WCHAR*		pszPath;
	BOOL		bResult;
	UINT		uLen;
	UINT		uNum;
	va_list		vl;

	//--- R�cup�re le r�pertoire temporaire du syst�me
	pszTempPath = Divine_GetTempPath(0);
	if (!pszTempPath) return(NULL);

	//--- Calcul la taille finale du r�pertoire temporaire
	va_start(vl,uNumPaths);
	uLen = wcslen(pszTempPath);
	uNum = uNumPaths;
	while (uNum)
		{
		uLen += 1+wcslen(va_arg(vl,WCHAR *));
		uNum--;
		}
	va_end(vl);

	//--- Alloue la m�moire pour le r�pertoire temporaire
	pszPath = HeapAlloc(App.hHeap,0,uLen*sizeof(WCHAR)+sizeof(WCHAR));
	if (!pszPath)
		{
		HeapFree(App.hHeap,0,pszTempPath);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return(NULL);
		}

	//--- Boucle de cr�ation des r�pertoires
	va_start(vl,uNumPaths);
	wcscpy(pszPath,pszTempPath);
	bResult = 1;
	while(uNumPaths)
		{
		PathAppend(pszPath,va_arg(vl,WCHAR *));
		if (!PathFileExists(pszPath))
			{
			bResult = CreateDirectory(pszPath,NULL);
			if (!bResult)
				{
				HeapFree(App.hHeap,0,pszPath);
				pszPath = NULL;
				break;
				}
			}
		uNumPaths--;
		}
	va_end(vl);

	HeapFree(App.hHeap,0,pszTempPath);
	return(pszPath);
}


// ���� D�termine un chemin pour un fichier/dossier temporaire �����������

WCHAR*	Divine_GetTempPath(UINT uNumPaths, ...)
{
	WCHAR*	pszPath;
	UINT	uNum;
	UINT	uLen;
	va_list	vl;

	//--- Calcul la taille totale du r�pertoire temporaire
	va_start(vl,uNumPaths);
	uLen = wcslen(App.Config.pszTempPath);
	uNum = uNumPaths;
	while (uNum)
		{
		uLen += 1+wcslen(va_arg(vl,WCHAR *));
		uNum--;
		}
	va_end(vl);

	//--- Alloue la m�moire pour le r�pertoire temporaire
	pszPath = HeapAlloc(App.hHeap,0,uLen*sizeof(WCHAR)+sizeof(WCHAR));
	if (!pszPath)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return(NULL);
		}

	//--- Copie du r�pertoire temporaire
	va_start(vl,uNumPaths);
	wcscpy(pszPath,App.Config.pszTempPath);
	while (uNumPaths)
		{
		PathAppend(pszPath,va_arg(vl,WCHAR *));
		uNumPaths--;
		}
	return(pszPath);
}


// ���� Recup�re le chemin du fichier de sauvegarde ����������������������

WCHAR* Divine_GetSaveGamePath(UINT uGame, WCHAR *pszProfile, WCHAR *pszSaveName)
{
	WCHAR*	pszPath;
	UINT	uLen;

	uLen  = wcslen(App.Config.pszLarianPath)*sizeof(WCHAR);
	uLen += sizeof(WCHAR)+wcslen(Divine_GetGameName(uGame))*sizeof(WCHAR);
	uLen += sizeof(WCHAR)+wcslen(szPlayerProfiles)*sizeof(WCHAR);
	uLen += sizeof(WCHAR)+wcslen(pszProfile)*sizeof(WCHAR);
	uLen += sizeof(WCHAR)+wcslen(szSavegames)*sizeof(WCHAR);
	uLen += sizeof(WCHAR)+wcslen(szStory)*sizeof(WCHAR);
	uLen += sizeof(WCHAR)+wcslen(pszSaveName)*sizeof(WCHAR);
	uLen += sizeof(WCHAR)+wcslen(pszSaveName)*sizeof(WCHAR);
	uLen += wcslen(szLSVext)*sizeof(WCHAR);
	pszPath = HeapAlloc(App.hHeap,0,uLen+sizeof(WCHAR));
	if (!pszPath)
		{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return(NULL);
		}
	wcscpy(pszPath,App.Config.pszLarianPath);
	PathAppend(pszPath,Divine_GetGameName(uGame));
	PathAppend(pszPath,szPlayerProfiles);
	PathAppend(pszPath,pszProfile);
	PathAppend(pszPath,szSavegames);
	PathAppend(pszPath,szStory);
	PathAppend(pszPath,pszSaveName);
	PathAppend(pszPath,pszSaveName);
	wcscat(pszPath,szLSVext);

	return(pszPath);
}


// ���� Recup�re le nom du jeu �������������������������������������������

//--- A partir d'un identifiant ---

WCHAR* Divine_GetGameName(UINT uGame)
{
	return(uGame == DIVINE_DOS_2?szGameName:szGameNameEE);
}

//--- A partir d'un nom ---

UINT Divine_GetGameFromName(WCHAR *pszName, UINT uLen)
{
	if (uLen == wcslen(szGameNameEE) && !wcsncmp(szGameNameEE,pszName,uLen)) return(DIVINE_DOS_2EE);
	return(DIVINE_DOS_2);
}


// ���� Ex�cute le programme de conversion �������������������������������

void Divine_RunConverter()
{
	if (!PathFileExists(szConverterEXE))
		{
		MessageBox(App.hWnd,Locale_GetText(TEXT_ERR_MISSINGCONVERTER),NULL,MB_ICONERROR);
		return;
		}

	ShellExecute(App.hWnd,L"open",szConverterEXE,NULL,NULL,SW_SHOWNORMAL);
	return;
}