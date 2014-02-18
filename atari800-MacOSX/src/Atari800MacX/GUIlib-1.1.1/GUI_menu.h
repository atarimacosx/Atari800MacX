#ifndef _GUI_menu_h
#define _GUI_menu_h


////////////////////////////////////////////////////////////////////////////////////////////////////
//  so far, only 10 submenus each with 10 items can be used  //
//  a dynamical allocation will follow in future //
///////////////////////////////////////////////////////////////////////


class GUI_Menu;
class GUI_Menuitem;

#include <string.h>

#include "GUI.h"
#include "GUI_area.h"
#include "GUI_button.h"


/* additional space horizontally and vertically of each item and submenu*/
#define MENU_OVERSIZE 10


/* This is the definition of the "I've been activated" callback */
typedef GUI_status (*GUI_MenuActiveProc)(int caller_id, int is_checked, void *data);


class GUI_Submenu : public GUI_Button
{

 public:

  GUI_Submenu(GUI_Menu *Aparent, int Asubmenuid, int x, int y, char *Atext,
	      GUI_Font *Afont, int is_checkmenu);

  virtual ~GUI_Submenu();

  virtual void AddSubitem(GUI_Menuitem *newitem);
  virtual GUI_Menuitem* GetSubItem(int Aid);

  GUI_status MouseDown(int x, int y, int button);
  GUI_status MouseUp(int x,int y,int button);
  GUI_status MouseMotion(int x,int y,Uint8 state);

  inline virtual int GetSubmenuId()
    {return submenuid;}
  inline virtual int GetNumSubitems()
    {return numitems;}
  inline virtual int GetLength()
    {return strlen(Text);}
  inline virtual char* GetText()
    {return Text;}

  virtual void SetItemsClickState(int button, int value);

 protected:
  int submenuid,id;
  char Text[64];
  GUI_MenuActiveProc MenuActiveProc;

  int numitems;
  GUI_Menuitem *items[10];

  GUI_Menu *parent;
};

//////////////////////////////////////////////////////////////////////////

class GUI_Menuitem : public GUI_Submenu
{

 public:
  GUI_Menuitem(GUI_Menu *Aparent, int Asubmenuid, int Aid, int x, int y,
	       char *Atext, GUI_Font *Afont,
	       GUI_MenuActiveProc Aactiveproc, int is_checkmenu = 0);

  inline virtual int GetId()
    {return id;}

  virtual void SetCheck(int flag);
  inline virtual int Checked()
    {return checked;}

};

///////////////////////////////////////////////////////////////////////////////////////

class GUI_Menu : public GUI_Area
{

public:

  /* Passed the GUI in which the Menu shall be running, the total width and a font */
  GUI_Menu(GUI *Agui, int width, GUI_Font *Afont = NULL);
  ~GUI_Menu();

  /* add a toplevel menu with an id and the caption */
  virtual void AddSubmenu(int Aid, char *Atext);
  /* add an item below the given submenu with an idm a caption, a callback and
     a flag if it shall be a checkable menu item */
  virtual void AddMenuitem(int Asubmenuid, int Aid, char *Atext,
	       GUI_MenuActiveProc activeproc, int is_checkmenu = 0);

  /* the menu items call this to keep track of each other - not very nice, I know*/
  virtual void SetCommonClickState(int submenuid, int button, int value);

protected:

  GUI *gui;
  GUI_Font *font;
  int numitems;
  GUI_Submenu *items[10];
};

#endif /* _GUI_menu_h */
