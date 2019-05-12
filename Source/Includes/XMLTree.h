
//<<>-<>>---------------------------------------------------------------------()
/*
	Structures XML
									      */
//()-------------------------------------------------------------------<<>-<>>//

#ifndef _XMLTREE_DEFINITIONS
#define _XMLTREE_DEFINITIONS


// いいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいい //
// い�									  い� //
// い� D�finitions							  い� //
// い�									  い� //
// いいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいい //

typedef struct XMLTREE {
	HWND		hWnd;
	HWND		hwndTree;
	WCHAR*		pszWindowTitle;
	XML_NODE*	pxnRoot;
} XMLTREE;


// いいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいい //
// い�									  い� //
// い� Prototypes							  い� //
// い�									  い� //
// いいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいいい //

void			Tree_Open(XML_NODE *);
LRESULT			Tree_Create(HWND);
void			Tree_Destroy(void);

LRESULT			Tree_ProcessMessages(HWND,UINT,WPARAM,LPARAM);

int			Tree_CreateTreeView(XML_NODE *);
int			Tree_CreateNodeTree(XML_NODE *,HTREEITEM);
int			Tree_CreateAttrTree(XML_NODE *,HTREEITEM);
void			Tree_DestroyTreeView(void);

#endif
