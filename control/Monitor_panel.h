/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  Monitor                          1
#define  Monitor_StartStreaming           2       /* control type: command, callback function: MyStartStreaming */
#define  Monitor_Intervals                3       /* control type: numeric, callback function: (none) */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

#define  MENUBAR                          1
#define  MENUBAR_onnection                2
#define  MENUBAR_onnection_Refresh        3       /* callback function: MyRefresh */
#define  MENUBAR_onnection_Disconnect     4       /* callback function: MyDisconnect */
#define  MENUBAR_onnection_Close          5       /* callback function: MyMenuQuit */
#define  MENUBAR_Log                      6
#define  MENUBAR_Log_Start_Loging         7       /* callback function: MyStartLog */
#define  MENUBAR_Log_OpenLog              8       /* callback function: MyExportLog */


     /* Callback Prototypes: */

void CVICALLBACK MyDisconnect(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK MyExportLog(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK MyMenuQuit(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK MyRefresh(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK MyStartLog(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK MyStartStreaming(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
