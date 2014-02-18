#include <string.h>

#include "GUI.h"
#include "GUI_font.h"
#include "GUI_button.h"
#include "GUI_menu.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
//  so far, only 10 submenus each with 10 items can be used  //
//  a dynamical allocation will follow in future //
///////////////////////////////////////////////////////////////////////


/* The default callback quits the GUI */
static GUI_status Default_MenuActiveProc(int caller_id, int is_checked, void *unused)
{
  return(GUI_QUIT);
}


GUI_Submenu::GUI_Submenu(GUI_Menu *Aparent, int Asubmenuid, int x, int y,
			 char *Atext, GUI_Font *Afont, int is_checkmenu)
  : GUI_Button(NULL,x,y,(strlen(Atext)+is_checkmenu*2)*Afont->CharWidth()+(MENU_OVERSIZE << 1),
	       Afont->CharHeight()+MENU_OVERSIZE,Atext,Afont,
	       BUTTON_TEXTALIGN_LEFT,is_checkmenu,
		   250,250,250,0,0,0,NULL,1)
{
  submenuid=Asubmenuid;
  id=-1;
  numitems=0;
  strcpy(Text,Atext);
  parent=Aparent;
}

GUI_Submenu::~GUI_Submenu()
{
// future: free dynamic items array
}

void GUI_Submenu::AddSubitem(GUI_Menuitem *newitem)
{
  if (numitems<10)
  {
    int maxLength=0;
    int i;

    for (i=0;i<numitems;i++)
      if (items[i]->GetLength()>maxLength)
        maxLength=items[i]->GetLength();

    items[numitems++]=newitem;
    if (newitem->GetLength()+newitem->IsCheckButton()*2>maxLength)
    {
      for (i=0;i<numitems;i++)
        items[i]->ChangeTextButton(-1,-1,(newitem->GetLength()+newitem->IsCheckButton()*2)*
			 buttonFont->CharWidth()+MENU_OVERSIZE,
			 -1,items[i]->GetText(), BUTTON_TEXTALIGN_LEFT);
    }
    if (newitem->GetLength()+newitem->IsCheckButton()*2<maxLength)
    {
      newitem->ChangeTextButton(-1,-1,maxLength*
		 buttonFont->CharWidth()+MENU_OVERSIZE,
		 -1,newitem->GetText(), BUTTON_TEXTALIGN_LEFT);
    }
    newitem->Hide();
  }
}

GUI_Menuitem* GUI_Submenu::GetSubItem(int Aid)
{
  int i=0;
  while ((i<numitems) && (items[i]->GetId()!=Aid))
    i++;

  if (i>=numitems) return NULL;
  return items[i];
}

/* Mouse hits activate us */
GUI_status GUI_Submenu:: MouseDown(int x, int y, int button)
{
  if (enabled &&(button==1))
  {
    parent->SetCommonClickState(submenuid,1,1);
    pressed[0]=1;
    Redraw();
  }
  return GUI_PASS;
}

GUI_status GUI_Submenu::MouseUp(int x,int y,int button)
{
  if ((button==1) && (pressed[0]==1))
  {
    parent->SetCommonClickState(submenuid,1,0);
    if ((x>=0) && (y>=0))
    {
    if (is_checkable)
      checked=!checked;
    if ((id>=0) &&(MenuActiveProc!=Default_MenuActiveProc))
      if (MenuActiveProc(id,checked,widget_data)==GUI_QUIT)
        return GUI_QUIT;
    }
/************ see comment below **********************************************/
    return GUI_REDRAW;
  }
  return GUI_PASS;
}

GUI_status GUI_Submenu::MouseMotion(int x,int y,Uint8 state)
{
  if ((pressed[0]==2) && (x>=0) && (y>=0))
  {
    parent->SetCommonClickState(submenuid,1,1);
    pressed[0]=1;
/* Unfortunately, here a complete GUI redraw is necessary because of the *********
** background restauration. In future there should be a background backup facility **
** in widgets. But to what extent? In a SDL program there should be action so any **
** background backup would certainly be out of date... ***************************/
    return GUI_REDRAW;
  }
  return GUI_PASS;
}

void GUI_Submenu::SetItemsClickState(int button, int value)
{
  for (int i=0;i<numitems;i++)
  {
    items[i]->SetClickState(button,value);
    if (value)
      items[i]->Show();
    else
      items[i]->Hide();
    items[i]->Redraw();
  }
}

//////////////////////////////////////////////////////////////////////////////////////

GUI_Menuitem::GUI_Menuitem(GUI_Menu *Aparent,int Asubmenuid, int Aid,
			   int x, int y, char *Atext, GUI_Font *Afont,
			   GUI_MenuActiveProc activeproc, int is_checkmenu)
  : GUI_Submenu(Aparent,Asubmenuid,x,y,Atext,Afont,is_checkmenu)
{
  id=Aid;
  if (activeproc==NULL)
  {
    MenuActiveProc=Default_MenuActiveProc;
  }
  else
  {
    MenuActiveProc=activeproc;
  }
}

void GUI_Menuitem::SetCheck(int flag)
{
  checked=flag;
}

///////////////////////////////////////////////////////////////////////////////////////

GUI_Menu::GUI_Menu(GUI *Agui, int width, GUI_Font *Afont)
  : GUI_Area(0,0,width,
       (Afont==NULL)?(8+MENU_OVERSIZE):(Afont->CharHeight()+MENU_OVERSIZE),
       220,220,220)
{
  gui=Agui;
  numitems=0;
  if (Afont==NULL)
    font=new GUI_Font();
  else
    font=Afont;
}

GUI_Menu::~GUI_Menu()
{
// future: free dynamic items array
}

void GUI_Menu::AddSubmenu(int Asubmenuid, char *Atext)
{
  int newpos=0;

  if (numitems<10)
  {
    for (int i=0;i<numitems;i++)
      newpos+=items[i]->W();

    items[numitems]=new GUI_Submenu(this,Asubmenuid,newpos,0,Atext,font,0);
    gui->AddWidget(items[numitems++]);
  }
}

void GUI_Menu::AddMenuitem(int Asubmenuid, int Aid, char *Atext,
	       GUI_MenuActiveProc Aactiveproc, int is_checkmenu)
{
  GUI_Submenu *temp=NULL;

  for (int i=0;i<numitems;i++)
    if (items[i]->GetSubmenuId()==Asubmenuid)
      temp=items[i];

  if (temp!=NULL)
  {
    GUI_Menuitem *newitem=new GUI_Menuitem(this,Asubmenuid, Aid, temp->X(),
       temp->GetNumSubitems()*(font->CharHeight()+MENU_OVERSIZE)+temp->H(),
       Atext,font,Aactiveproc,is_checkmenu);
    temp->AddSubitem(newitem);
    gui->AddWidget(newitem);
  }
}

void GUI_Menu::SetCommonClickState(int submenuid,int button, int value)
{
  if ((button>0) && (button<=3))
    pressed[button-1]=value;

  for (int i=0;i<numitems;i++)
  {
    if ((items[i]->GetSubmenuId()==submenuid) && (value>0))
    {
      items[i]->SetItemsClickState(button,2);
      items[i]->SetClickState(button,1);
      items[i]->Redraw();
    }
    else
    {
      items[i]->SetItemsClickState(button,0);
      items[i]->SetClickState(button,(value>0)?2:0);
      items[i]->Redraw();
    }
  }
}
