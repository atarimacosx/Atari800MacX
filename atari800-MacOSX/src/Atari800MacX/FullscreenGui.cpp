/* FullScreenGui.h - FullScreenGui 
 module  For the Macintosh OS X SDL port of Atari800
 Mark Grebe <atarimac@kc.rr.com>
 
 */


#include "FullscreenGui.h"
#include "string.h"
#include "arrows.h"

extern SDL_Surface *MainScreen;
extern SDL_Surface *MonitorGLScreen;
extern int OPENGL;
extern int MONITOR_assemblerMode;
extern int MonitorScreenID;
extern int MainScreenID;
extern int fullscreenWidth;
extern int fullscreenHeight;
extern int fullForeRed;
extern int fullForeGreen;
extern int fullForeBlue;
extern int fullBackRed;
extern int fullBackGreen;
extern int fullBackBlue;
extern int fullscreenGuiColorsChanged;
extern int fullscreenCrashColorsChanged;
extern "C" {
	void MONITOR_monitorEnter(void);
	int MONITOR_monitorCmd(char *input);
	void ControlManagerAddToHistory(char *string);
	char *ControlManagerGetHistoryString(int direction);
	void NSBeep(void);
	}

static GUI *fullscreenGUI = NULL;
static GUI *fullscreenCrashGUI = NULL;
static GUI_TermWin *terminal = NULL;
static GUI_ScrollButtons *scrollbar = NULL;
static char inputBuffer[256];
static int inputLen = 0;
static int inputPos = 0;
static int monitorRetValue;
static int monitorGUIOpenGL;
static int crashGUIOpenGL;
static int crashStatus;

SDL_Surface *upimage;
SDL_Surface *downimage;

#define MONITOR_WIDTH 640
#define MONITOR_HEIGHT 480

int keyFunc (SDLKey key, Uint16 unicode)
{
	int i;
	char *historyLine;
	
	if (unicode >=0x20 && unicode <= 0x7e) {
		if (inputPos == 256) {
			NSBeep();
			return(0);
			}
		if (inputPos < inputLen) {
			for (i=inputLen-1;i>=inputPos;i--)
				inputBuffer[i+1] = inputBuffer[i];
			}
		inputBuffer[inputPos] = (char) unicode;
		inputLen++;
		inputPos++;
		terminal->AddPromptText(inputBuffer,inputLen,inputPos);
		return(0);
		}
	else if (unicode == 0x7f) {
		if (inputPos) {
			for (i=inputPos;i<inputLen;i++)
				inputBuffer[i] = inputBuffer[i+1];
			inputPos--;
			inputLen--;
			terminal->AddPromptText(inputBuffer,inputLen,inputPos);
			}
		return(0);
		}
	else if (unicode == 0xf728) {
		if (inputPos < inputLen) {
			for (i=inputPos;i<inputLen;i++)
				inputBuffer[i] = inputBuffer[i+1];
			inputLen--;
			terminal->AddPromptText(inputBuffer,inputLen,inputPos);
			}
		return(0);
		}
	else if (unicode == 0xf702) {
		if (inputPos) {
			inputPos--;
			terminal->AddPromptText(inputBuffer,inputLen,inputPos);
			}
		return(0);
		}
	else if (unicode == 0xf703) {
		if (inputPos < inputLen) {
			inputPos++;
			terminal->AddPromptText(inputBuffer,inputLen,inputPos);
			}
		return(0);
		}
	else if (unicode == 0xf700) {
		historyLine = ControlManagerGetHistoryString(1);
		if (historyLine == NULL)
			return(0);
		else {
			strncpy(inputBuffer,historyLine,255);
			inputLen = strlen(inputBuffer);
			inputPos = inputLen;
			terminal->AddPromptText(inputBuffer,inputLen,inputPos);
			return(0);
			}
		}
	else if (unicode == 0xf701) {
		historyLine = ControlManagerGetHistoryString(-1);
		if (historyLine == NULL)
			return(0);
		else {
			strncpy(inputBuffer,historyLine,255);
			inputLen = strlen(inputBuffer);
			inputPos = inputLen;
			terminal->AddPromptText(inputBuffer,inputLen,inputPos);
			return(0);
			}
		}
	else if (unicode == 0x0d) {
		terminal->AddPromptText(inputBuffer,inputLen,inputLen);
		terminal->AddText("\n");
		inputBuffer[inputLen] = 0;
		
		ControlManagerAddToHistory(inputBuffer);
		monitorRetValue = MONITOR_monitorCmd(inputBuffer); 
		if (!MONITOR_assemblerMode && monitorRetValue>-1)
			terminal->AddText(">");
		inputLen = 0;
		inputPos = 0;
		terminal->StartPrompt();

		return(monitorRetValue);
	}
	return(0);		
}

static GUI_status Quit_ActiveProc(void *unused)
{
	crashStatus = 0;
	return(GUI_QUIT);
}

static GUI_status Monitor_ActiveProc(void *unused)
{
	crashStatus = 3;
	return(GUI_QUIT);
}

static GUI_status Coldstart_ActiveProc(void *unused)
{
	crashStatus = 2;
	return(GUI_QUIT);
}

static GUI_status Warmstart_ActiveProc(void *unused)
{
	crashStatus = 1;
	return(GUI_QUIT);
}

static GUI_status EjectCart_ActiveProc(void *unused)
{
	crashStatus = 4;
	return(GUI_QUIT);
}

static GUI_status EjectDisk_ActiveProc(void *unused)
{
	crashStatus = 5;
	return(GUI_QUIT);
}

static void setSurfaceColors(SDL_Surface *surface,
				Uint8 fr,Uint8 fg,Uint8 fb, int bg_opaque,
				Uint8 br, Uint8 bg, Uint8 bb)
{				
	SDL_Color colors[3]={{br,bg,bb,0},{fr,fg,fb,0}};
	if (bg_opaque)
	{
	  SDL_SetColors(surface,colors,0,2);
	  SDL_SetColorKey(surface,0,0);
	}
	else
	{
	  SDL_SetColors(surface,&colors[1],1,1);
	  SDL_SetColorKey(surface,SDL_SRCCOLORKEY,0);
	}
}

static void initFullscreenGUI(SDL_Surface *display, int openGl)
{
	SDL_Rect middle_rect;
	Uint32 fg,bg;

	fullscreenGUI = new GUI(display, MONITOR_WIDTH, MONITOR_HEIGHT, openGl, MainScreenID);

	fg = SDL_MapRGBA(display->format,fullForeRed,fullForeGreen,fullForeBlue,0);
	bg = SDL_MapRGBA(display->format,fullBackRed,fullBackGreen,fullBackBlue,0);
	
	upimage = GUI_LoadImage(arrow_w, arrow_h, arrow_pal, up_data);
	setSurfaceColors(upimage,fullForeRed,fullForeGreen,fullForeBlue,1,fullBackRed,fullBackGreen,fullBackBlue);

	downimage = GUI_LoadImage(arrow_w, arrow_h, arrow_pal, down_data);
	setSurfaceColors(downimage,fullForeRed,fullForeGreen,fullForeBlue,1,fullBackRed,fullBackGreen,fullBackBlue);		
	
	terminal = new GUI_TermWin(0,0,MONITOR_WIDTH-upimage->w,MONITOR_HEIGHT,NULL,keyFunc,240);
	fullscreenGUI->AddWidget(terminal);
	
	middle_rect.x = MONITOR_WIDTH-upimage->w;
	middle_rect.y = upimage->h;
	middle_rect.w = upimage->w;
	middle_rect.h = MONITOR_HEIGHT-upimage->h-downimage->h;
	scrollbar = new GUI_ScrollButtons(MONITOR_WIDTH-upimage->w,0, upimage, 
									  middle_rect,
									  MONITOR_WIDTH-downimage->w, MONITOR_HEIGHT-downimage->h, downimage, 
									  SCROLLBAR_VERTICAL, 0.25, terminal);
	SDL_FillRect(display, &middle_rect, fg);
	middle_rect.x++; middle_rect.y++, middle_rect.w -= 2; middle_rect.h -= 2;
	SDL_FillRect(display, &middle_rect, bg);
	fullscreenGUI->AddWidget(scrollbar);
	
    terminal->AddText("Atari800MacX Monitor\nType '?' for help, 'CONT' to exit\n");	
	terminal->SetColoring(fullForeRed,fullForeGreen,fullForeBlue,1,fullBackRed,fullBackGreen,fullBackBlue);
	scrollbar->SetColoring(fullForeRed,fullForeGreen,fullForeBlue,1,fullBackRed,fullBackGreen,fullBackBlue);
}

static void initFullscreenCrashGUI(SDL_Surface *display, int openGl)
{
	GUI_Area *area;
	GUI_Button *button;
	GUI_TermWin *textArea;

	fullscreenCrashGUI = new GUI(display, MONITOR_WIDTH, MONITOR_HEIGHT, openGl, MainScreenID);

	area = new GUI_Area(70, 140, 500, 150,fullBackRed,fullBackGreen,fullBackBlue, 
						fullForeRed,fullForeGreen,fullForeBlue, 2,AREA_ANGULAR);
	fullscreenCrashGUI->AddWidget(area);

	textArea = new GUI_TermWin(80, 170, 480, 80, NULL, NULL, 32);
	textArea->AddText("Unrecoverable Error\n\n");
	textArea->AddText("The Atari800MacX Emulator had detected an unrecoverable\n");
	textArea->AddText("error.  Please select ColdStart or WarmStart to attempt\n");
	textArea->AddText("restart, Monitor to enter the monitor, Eject to eject\n");
	textArea->AddText("Cartridge or Boot Disk, or Quit to exit the emulator.");
	fullscreenCrashGUI->AddWidget(textArea);	
	textArea->SetColoring(fullForeRed,fullForeGreen,fullForeBlue,1,fullBackRed,fullBackGreen,fullBackBlue);
	
	button = new GUI_Button(NULL, 80, 230,120,15,"Quit Emulator",NULL,
							BUTTON_TEXTALIGN_CENTER,0,
							fullForeRed,fullForeGreen,fullForeBlue,fullBackRed,fullBackGreen,fullBackBlue,
							Quit_ActiveProc,0);
	fullscreenCrashGUI->AddWidget(button);
	button = new GUI_Button(NULL, 220, 230,100,15,"Monitor",NULL,
							BUTTON_TEXTALIGN_CENTER,0,
							fullForeRed,fullForeGreen,fullForeBlue,fullBackRed,fullBackGreen,fullBackBlue,
							Monitor_ActiveProc,0);
	fullscreenCrashGUI->AddWidget(button);
	button = new GUI_Button(NULL, 340, 230,100,15,"ColdStart",NULL,
							BUTTON_TEXTALIGN_CENTER,0,
							fullForeRed,fullForeGreen,fullForeBlue,fullBackRed,fullBackGreen,fullBackBlue,
							Coldstart_ActiveProc,0);
	fullscreenCrashGUI->AddWidget(button);
	button = new GUI_Button(NULL, 455, 230,100,15,"WarmStart",NULL,
							BUTTON_TEXTALIGN_CENTER,0,
							fullForeRed,fullForeGreen,fullForeBlue,fullBackRed,fullBackGreen,fullBackBlue,
							Warmstart_ActiveProc,0);
	fullscreenCrashGUI->AddWidget(button);
	button = new GUI_Button(NULL, 145, 250,150,15,"Eject Cartridge",NULL,
							BUTTON_TEXTALIGN_CENTER,0,
							fullForeRed,fullForeGreen,fullForeBlue,fullBackRed,fullBackGreen,fullBackBlue,
							EjectCart_ActiveProc,0);
	fullscreenCrashGUI->AddWidget(button);
	button = new GUI_Button(NULL, 315, 250,150,15,"Eject Disk 1",NULL,
							BUTTON_TEXTALIGN_CENTER,0,
							fullForeRed,fullForeGreen,fullForeBlue,fullBackRed,fullBackGreen,fullBackBlue,
							EjectDisk_ActiveProc,0);
	fullscreenCrashGUI->AddWidget(button);
	
}

int FullscreenCrashGUIRun(void)
{
	if (fullscreenCrashGUI == NULL) {
		crashGUIOpenGL = OPENGL;
		if (OPENGL)
			initFullscreenCrashGUI(MonitorGLScreen, 1);
		else
			initFullscreenCrashGUI(MainScreen, 0);		
		}
	else {
		if (crashGUIOpenGL != OPENGL || fullscreenCrashColorsChanged) {
			delete(fullscreenCrashGUI);
			if (OPENGL)
				initFullscreenCrashGUI(MonitorGLScreen, 1);
			else
				initFullscreenCrashGUI(MainScreen, 0);	
			}
		crashGUIOpenGL = OPENGL;
		fullscreenCrashColorsChanged = 0;
		}
		
	/* Set the scaling for the monitor */
	if (OPENGL) {
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0, (GLdouble) MONITOR_WIDTH, (GLdouble) MONITOR_HEIGHT, 0.0, 0.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		}

	SDL_ShowCursor(SDL_ENABLE); // show mouse cursor 
	fullscreenCrashGUI->Run();
	SDL_ShowCursor(SDL_DISABLE); // show mouse cursor 
	
	/* Set the scaling for the normal Atari Screen */
	if (OPENGL) {
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0,(GLdouble) fullscreenWidth, (GLdouble) fullscreenHeight, 0.0, 0.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		}
	
	return(crashStatus);
}

int FullscreenGUIRun(void)
{
	if (fullscreenGUI == NULL) {
		monitorGUIOpenGL = OPENGL;
		if (OPENGL)
			initFullscreenGUI(MonitorGLScreen, 1);
		else
			initFullscreenGUI(MainScreen, 0);		
		}
	else {
		if (monitorGUIOpenGL != OPENGL || fullscreenGuiColorsChanged) {
			delete(fullscreenGUI);
			SDL_FreeSurface(upimage);
			SDL_FreeSurface(downimage);
			if (OPENGL)
				initFullscreenGUI(MonitorGLScreen, 1);
			else
				initFullscreenGUI(MainScreen, 0);
			}
		monitorGUIOpenGL = OPENGL;
		fullscreenGuiColorsChanged = 0;
		}
		
	/* Set the scaling for the monitor */
	if (OPENGL) {
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0, (GLdouble) MONITOR_WIDTH, (GLdouble) MONITOR_HEIGHT, 0.0, 0.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		}

	MONITOR_monitorEnter();
	terminal->AddText(">");
	terminal->StartPrompt();
	SDL_ShowCursor(SDL_ENABLE); // show mouse cursor 
	fullscreenGUI->Run();
	SDL_ShowCursor(SDL_DISABLE); // show mouse cursor 
	
	/* Set the scaling for the normal Atari Screen */
	if (OPENGL) {
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0,(GLdouble) fullscreenWidth, (GLdouble) fullscreenHeight, 0.0, 0.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		}
    if (monitorRetValue <= -1)
        return(1);
    else
        return(0);
}

void FullscreenGUIPrintf(const char *format,...)
{
	static char string[512];
	va_list arguments;             
    va_start(arguments, format);  
    
	vsprintf(string, format, arguments);
    terminal->AddText(string);
}

void FullscreenGUIPuts(const char *string)
{
    terminal->AddText(string,strlen(string));
}


