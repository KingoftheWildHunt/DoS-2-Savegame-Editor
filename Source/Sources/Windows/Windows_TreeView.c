
//<<>-<>>---------------------------------------------------------------------()
/*
	Liste XML sous forme de TreeView (Debug principalement)
									      */
//()-------------------------------------------------------------------<<>-<>>//

// いいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいい //
// い�									  い� //
// い� Donn�es								  い� //
// い�									  い� //
// いいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいい //

#include "Application.h"
#include "XML.h"
#include "XMLTree.h"
#include "Texts.h"
#include "Requests.h"
#include "Utils.h"

extern APPLICATION		App;


// いいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいい //
// い�									  い� //
// い� Initalisations							  い� //
// い�									  い� //
// いいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいい //

// �算� Cr�ation de la fen�tre �������������������������������������������

void Tree_Open(XML_NODE *pxn)
{
	HWND		hWnd;
	HMONITOR	hMonitor;
	MONITORINFO	Info;
	int		X,Y;

	Tree_Destroy();
	App.xmlTree.pxn = pxn;

	hMonitor = MonitorFromWindow(NULL,MONITOR_DEFAULTTOPRIMARY);
	Info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor,&Info);
	X = ((Info.rcMonitor.right-Info.rcMonitor.left)-800)/2;
	Y = ((Info.rcMonitor.bottom-Info.rcMonitor.top)-700)/2;

	hWnd = CreateWindowEx(0,szWindowClass,szTitle,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,X,Y,800,700,NULL,NULL,App.hInstance,(void *)WINDOW_TREE);
	if (!hWnd)
		{
		App.xmlTree.pxn = NULL;
		App.xmlTree.hWnd = NULL;
		Request_PrintError(App.hWnd,Locale_GetText(TEXT_ERR_WINDOW_CREATE),NULL,MB_ICONERROR);
		return;
		}

	ShowWindow(hWnd,SW_SHOWNORMAL);
	UpdateWindow(hWnd);
	return;
}

//--- WM_CREATE ---

LRESULT Tree_Create(HWND hWnd)
{
	RECT	rcClient;
	int	X,Y,W,H;

	App.xmlTree.hWnd = hWnd;

	W = 150;
	H = App.Font.uFontHeight+16;

	GetClientRect(hWnd,&rcClient);
	rcClient.left += 16;
	rcClient.right -= 16;
	rcClient.top += 16;
	rcClient.bottom -= 16+H+14;

	App.xmlTree.hwndTree = CreateWindowEx(WS_EX_STATICEDGE,WC_TREEVIEW,NULL,WS_CHILD|TVS_DISABLEDRAGDROP|TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|TVS_SHOWSELALWAYS|TVS_EDITLABELS,rcClient.left,rcClient.top,rcClient.right-rcClient.left,rcClient.bottom-rcClient.top,hWnd,(HMENU)CTLID_TREEVIEW,App.hInstance,NULL);
	if (!App.xmlTree.hwndTree) return(0);
	if (!Tree_CreateTreeView(App.xmlTree.pxn)) return(0);

	X = rcClient.right-W-4;
	Y = rcClient.bottom+14;

	App.xmlTree.hwndClose = CreateWindowEx(0,szButtonClass,NULL,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON|BS_TEXT|BS_CENTER|BS_VCENTER,X,Y,W,H,hWnd,(HMENU)(UINT_PTR)CTLID_TREECLOSE,App.hInstance,NULL);
	if (!App.xmlTree.hwndClose) return(0);
	App.xmlTree.hwndHelp = CreateWindowEx(0,szButtonClass,NULL,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON|BS_TEXT|BS_CENTER|BS_VCENTER,X-W-8,Y,W,H,hWnd,(HMENU)(UINT_PTR)CTLID_TREEHELP,App.hInstance,NULL);
	if (!App.xmlTree.hwndHelp) return(0);
	App.xmlTree.hwndSave = CreateWindowEx(0,szButtonClass,NULL,WS_CHILD|WS_VISIBLE|WS_DISABLED|WS_TABSTOP|BS_PUSHBUTTON|BS_TEXT|BS_CENTER|BS_VCENTER,X-W-8-W-8,Y,W,H,hWnd,(HMENU)(UINT_PTR)CTLID_TREESAVE,App.hInstance,NULL);
	if (!App.xmlTree.hwndSave) return(0);

	SendMessage(App.xmlTree.hwndClose,WM_SETFONT,(WPARAM)App.Font.hFont,0);
	SendMessage(App.xmlTree.hwndClose,WM_SETTEXT,0,(LPARAM)Locale_GetText(TEXT_CLOSE));
	SendMessage(App.xmlTree.hwndHelp,WM_SETFONT,(WPARAM)App.Font.hFont,0);
	SendMessage(App.xmlTree.hwndHelp,WM_SETTEXT,0,(LPARAM)Locale_GetText(TEXT_HELP));
	SendMessage(App.xmlTree.hwndSave,WM_SETFONT,(WPARAM)App.Font.hFont,0);
	SendMessage(App.xmlTree.hwndSave,WM_SETTEXT,0,(LPARAM)Locale_GetText(TEXT_SAVE));

	ShowWindow(App.xmlTree.hwndTree,SW_SHOWNORMAL);
	return(1);
}

//--- Destruction de la fen�tre ---

void Tree_Destroy()
{
	if (!IsWindow(App.xmlTree.hWnd)) return;
	SendMessage(App.xmlTree.hWnd,WM_CLOSE,0,0);
	return;
}


// �算� Cr�ation de la liste ���������������������������������������������

int Tree_CreateTreeView(XML_NODE *pxn)
{
	if (!pxn) return(1);

	if (!Tree_CreateNodeTree(pxn,NULL))
		{
		Tree_DestroyTreeView();
		Request_PrintError(App.hWnd,Locale_GetText(TEXT_ERR_TREE_CREATE),NULL,MB_ICONERROR);
		return(0);
		}

	if (pxn->parent)
		{
		WCHAR*	Text;
		Text = xml_AttrToWideChar((XML_NODE *)pxn->parent);
		if (Text)
			{
			Misc_SetWindowText(App.xmlTree.hWnd,&App.xmlTree.pszWindowTitle,szTitle,szTitleFmtLong,szTitle,Locale_GetText(TEXT_TITLE_TREE),Text);
			LocalFree(Text);
			}
		}
	else Misc_SetWindowText(App.xmlTree.hWnd,&App.xmlTree.pszWindowTitle,szTitle,szTitleFmt,szTitle,Locale_GetText(TEXT_TITLE_TREE));

	return(1);
}

//--- Cr�ation d'un noeud ---

int Tree_CreateNodeTree(XML_NODE *pxn, HTREEITEM hParent)
{
	XML_NODE*	pxnCurrent;
	TV_INSERTSTRUCT	tvItem;
	HTREEITEM	hItem;

	for (pxnCurrent = pxn; pxnCurrent != NULL; pxnCurrent = (XML_NODE *)pxnCurrent->node.next)
		{
		tvItem.hParent = hParent?hParent:TVI_ROOT;
		tvItem.hInsertAfter = TVI_LAST;
		tvItem.itemex.mask = TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT;
		tvItem.itemex.pszText = LPSTR_TEXTCALLBACK;
		tvItem.itemex.cChildren = (pxnCurrent->children.next || pxnCurrent->attributes.next)?1:0;
		tvItem.itemex.lParam = (LPARAM)pxnCurrent;
		hItem = (HTREEITEM)SendMessage(App.xmlTree.hwndTree,TVM_INSERTITEM,0,(LPARAM)&tvItem);
		if (!hItem) return(0);
		if (pxnCurrent->attributes.next) Tree_CreateAttrTree(pxnCurrent,hItem);
		if (pxnCurrent->children.next) { if (!Tree_CreateNodeTree((XML_NODE *)pxnCurrent->children.next,hItem)) return(0); }
		}

	return(1);
}

//--- Cr�ation des valeurs ---

int Tree_CreateAttrTree(XML_NODE *pxn, HTREEITEM hParent)
{
	XML_ATTR*	pxa;
	TV_INSERTSTRUCT	tvItem;
	HTREEITEM	hItem;

	for (pxa = (XML_ATTR *)pxn->attributes.next; pxa != NULL; pxa = (XML_ATTR *)pxa->node.next)
		{
		tvItem.hParent = hParent;
		tvItem.hInsertAfter = TVI_LAST;
		tvItem.itemex.mask = TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT;
		tvItem.itemex.pszText = LPSTR_TEXTCALLBACK;
		tvItem.itemex.cChildren = 0;
		tvItem.itemex.lParam = (LPARAM)pxa;
		hItem = (HTREEITEM)SendMessage(App.xmlTree.hwndTree,TVM_INSERTITEM,0,(LPARAM)&tvItem);
		if (!hItem) return(0);
		}

	return(1);
}


// �算� Destruction de la liste ������������������������������������������

void Tree_DestroyTreeView()
{
	Misc_SetWindowText(App.xmlTree.hWnd,&App.xmlTree.pszWindowTitle,szTitle,NULL);
	SendMessage(App.xmlTree.hwndTree,TVM_DELETEITEM,0,(LPARAM)TVI_ROOT);
	return;
}


// いいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいい //
// い�									  い� //
// い� Gestion des messages						  い� //
// い�									  い� //
// いいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいい //

LRESULT Tree_ProcessMessages(HWND hWnd, UINT uMsgId, WPARAM wParam, LPARAM lParam)
{
	switch(uMsgId)
		{
		case WM_PAINT:
			if (GetUpdateRect(hWnd,NULL,FALSE))
				{
				HDC		hDC;
				PAINTSTRUCT	paintStruct;
				RECT		rcWindow;

				hDC = BeginPaint(hWnd,&paintStruct);
				GetClientRect(hWnd,&rcWindow);
				FillRect(hDC,&rcWindow,GetSysColorBrush(COLOR_BTNFACE));
				EndPaint(hWnd,&paintStruct);
				}
			break;

		case WM_NOTIFY:
			switch(((NMHDR *)lParam)->code)
				{
				case TVN_KEYDOWN:
					if (((NMHDR *)lParam)->idFrom == CTLID_TREEVIEW)
						{
						HWND		hwndTree;
						HTREEITEM	hTreeItem;

						hwndTree = ((NMHDR *)lParam)->hwndFrom;

						switch(((TV_KEYDOWN *)lParam)->wVKey)
							{
							case VK_RETURN:
								hTreeItem = (HTREEITEM)SendMessage(hwndTree,TVM_GETNEXTITEM,TVGN_CARET,0);
								if (hTreeItem)
									{
									TVITEMEX	tvItem;

									tvItem.mask = TVIF_HANDLE|TVIF_PARAM;
									tvItem.hItem = hTreeItem;
									tvItem.lParam = 0;
									SendMessage(hwndTree,TVM_GETITEM,0,(LPARAM)&tvItem);
									if (tvItem.lParam) switch(((NODE *)tvItem.lParam)->type)
										{
										case XML_TYPE_NODE:
											SendMessage(hwndTree,TVM_EXPAND,TVE_TOGGLE,(LPARAM)hTreeItem);
											break;
										case XML_TYPE_ATTR:
											if (App.hThread) break;
											SendMessage(hwndTree,TVM_EDITLABEL,0,(LPARAM)hTreeItem);
											break;
										}
									}
								break;
							case VK_DELETE:
								if (App.hThread) break;
								if (MessageBox(hWnd,Locale_GetText(TEXT_NODES_REMOVE),Locale_GetText(TEXT_TITLE_REQUEST),MB_ICONQUESTION|MB_YESNO) == IDYES)
									{
									hTreeItem = (HTREEITEM)SendMessage(hwndTree,TVM_GETNEXTITEM,TVGN_CARET,0);
									if (hTreeItem)
										{
										TVITEMEX	tvItem;

										tvItem.mask = TVIF_HANDLE|TVIF_PARAM;
										tvItem.hItem = hTreeItem;
										tvItem.lParam = 0;
										SendMessage(hwndTree,TVM_GETITEM,0,(LPARAM)&tvItem);
										if (tvItem.lParam)
											{
											switch(((NODE *)tvItem.lParam)->type)
												{
												case XML_TYPE_NODE:
													xml_ReleaseNode((XML_NODE *)tvItem.lParam);
													break;
												case XML_TYPE_ATTR:
													xml_ReleaseAttr((XML_ATTR *)tvItem.lParam);
													break;
												}
											SendMessage(hwndTree,TVM_DELETEITEM,0,(LPARAM)hTreeItem);
											}
										}
									}
								SetFocus(App.xmlTree.hwndTree);
								break;
							case VK_F2:
								if (App.hThread) break;
								hTreeItem = (HTREEITEM)SendMessage(hwndTree,TVM_GETNEXTITEM,TVGN_CARET,0);
								if (hTreeItem) SendMessage(hwndTree,TVM_EDITLABEL,0,(LPARAM)hTreeItem);
								break;
							}
						}
					break;

				case TVN_BEGINLABELEDIT:
					if (((NMHDR *)lParam)->idFrom == CTLID_TREEVIEW)
						{
						NMTVDISPINFO*	pDispInfo;
						XML_ATTR*	pxa;
						HWND		hwndEdit;

						pDispInfo = (NMTVDISPINFO *)lParam;
						switch(((NODE *)pDispInfo->item.lParam)->type)
							{
							case XML_TYPE_NODE:
								return(1);
							case XML_TYPE_ATTR:
								pxa = (XML_ATTR *)pDispInfo->item.lParam;
								hwndEdit = (HWND)SendMessage(((NMHDR *)lParam)->hwndFrom,TVM_GETEDITCONTROL,0,0);
								SendMessage(hwndEdit,WM_SETTEXT,0,(LPARAM)pxa->value);
								break;
							}
						}
					break;

				case TVN_ENDLABELEDIT:
					if (((NMHDR *)lParam)->idFrom == CTLID_TREEVIEW)
						{
						NMTVDISPINFO*	pDispInfo;
						XML_ATTR*	pxa;
						WCHAR*		pszValue;

						pDispInfo = (NMTVDISPINFO *)lParam;
						pxa = (XML_ATTR *)pDispInfo->item.lParam;
						if (pDispInfo->item.pszText)
							{
							if (wcslen(pDispInfo->item.pszText))
								{
								pszValue = HeapAlloc(App.hHeap,0,wcslen(pDispInfo->item.pszText)*sizeof(WCHAR)+sizeof(WCHAR));
								if (pszValue)
									{
									if (pxa->value) HeapFree(App.hHeap,0,pxa->value);
									pxa->value = pszValue;
									wcscpy(pxa->value,pDispInfo->item.pszText);
									return(1);
									}
								}
							else
								{
								if (pxa->value) HeapFree(App.hHeap,0,pxa->value);
								pxa->value = NULL;
								return(1);
								}
							}
						}
					break;

				case TVN_GETDISPINFO:
					if (((NMHDR *)lParam)->idFrom == CTLID_TREEVIEW)
						{
						NMTVDISPINFO*	pDispInfo;
						XML_NODE*	pxn;
						XML_ATTR*	pxa;
						WCHAR*		pszText;
						int		iMax;

						pDispInfo = (NMTVDISPINFO *)lParam;
						iMax = pDispInfo->item.cchTextMax;
						switch(((NODE *)pDispInfo->item.lParam)->type)
							{
							case XML_TYPE_NODE:
								pxn = (XML_NODE *)pDispInfo->item.lParam;
								Misc_StrCpy(pDispInfo->item.pszText,iMax,pxn->name);
								pszText = xml_GetAttrValue(pxn,szXMLid);
								if (!pszText) break;
								Misc_StrCat(pDispInfo->item.pszText,iMax,L"[");
								Misc_StrCat(pDispInfo->item.pszText,iMax,szXMLid);
								Misc_StrCat(pDispInfo->item.pszText,iMax,L"=");
								Misc_StrCat(pDispInfo->item.pszText,iMax,pszText);
								Misc_StrCat(pDispInfo->item.pszText,iMax,L"]");
								break;
							case XML_TYPE_ATTR:
								pxa = (XML_ATTR *)pDispInfo->item.lParam;
								Misc_StrCpy(pDispInfo->item.pszText,iMax,pxa->name);
								Misc_StrCat(pDispInfo->item.pszText,iMax,L"=\"");
								if (pxa->value) Misc_StrCat(pDispInfo->item.pszText,iMax,pxa->value);
								Misc_StrCat(pDispInfo->item.pszText,iMax,L"\"");
								break;
							}
						}
					break;
				}
			break;

		case WM_ACTIVATE:
			switch(LOWORD(wParam))
				{
				case WA_ACTIVE:
				case WA_CLICKACTIVE:
					SetFocus(App.xmlTree.hwndTree);
					break;
				}
			break;

		case WM_COMMAND:
			switch(HIWORD(wParam))
				{
				case BN_CLICKED:
					switch(LOWORD(wParam))
						{
						case CTLID_TREEHELP:
							MessageBox(hWnd,Locale_GetText(TEXT_TREE_HELP),Locale_GetText(TEXT_TITLE_INFO),MB_ICONINFORMATION|MB_OK);
							break;
						case CTLID_TREECLOSE:
							PostMessage(App.xmlTree.hWnd,WM_CLOSE,0,0);
							break;
						}
					break;
				}
			break;

		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;

		case WM_DESTROY:
			Misc_SetWindowText(App.xmlTree.hWnd,&App.xmlTree.pszWindowTitle,szTitle,NULL);
			App.xmlTree.hWnd = NULL;
			break;

		default:
			return(DefWindowProc(hWnd,uMsgId,wParam,lParam));
		}

	return(0);
}
