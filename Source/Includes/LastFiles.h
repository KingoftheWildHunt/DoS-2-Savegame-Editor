
//<<>-<>>---------------------------------------------------------------------()
/*
	D嶨initions des fichiers r嶰ents
									      */
//()-------------------------------------------------------------------<<>-<>>//

#ifndef _LASTFILES_INCLUDE
#define _LASTFILES_INCLUDE

#include "Menus.h"


// 中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中 //
// 中�									  中� //
// 中� D嶨initions							  中� //
// 中�									  中� //
// 中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中 //

#define LASTFILES_MAX		12
#define IDM_LASTFILES		20000


// 中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中 //
// 中�									  中� //
// 中� Structures							  中� //
// 中�									  中� //
// 中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中 //

typedef struct LASTFILE {
	NODE			node;
	WCHAR*			pszPath;
	CUSTOMMENUTEMPLATE	cMenu;
} LASTFILE;


// 中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中 //
// 中�									  中� //
// 中� Prototypes							  中� //
// 中�									  中� //
// 中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中中 //

void			LastFiles_ReleaseAll(void);
void 			LastFiles_Release(LASTFILE *);
void 			LastFiles_Add(WCHAR *,WCHAR *,WCHAR *);
void			LastFiles_RemoveMenuItem(LASTFILE *);
void			LastFiles_InsertMenuItem(LASTFILE *);
void			LastFiles_AppendItems(void);
void			LastFiles_RemoveObsolete(void);
void			LastFiles_RemoveAll(void);

void			LastFiles_LoadList(void);
void			LastFiles_SaveList(void);
void			LastFiles_Reload(UINT);
int			LastFiles_Explode(WCHAR *,UINT *,WCHAR **,WCHAR **);

#endif
