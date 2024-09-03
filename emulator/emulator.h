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

#define  MONITOR                          1
#define  MONITOR_SendMSG                  2       /* control type: textBox, callback function: (none) */
#define  MONITOR_RecieveMSG               3       /* control type: textBox, callback function: (none) */

#define  PANEL                            2
#define  PANEL_QUITBUTTON                 2       /* control type: command, callback function: QuitCallback */
#define  PANEL_Add_Sensor                 3       /* control type: command, callback function: add_new_Sensor */
#define  PANEL_Sensor_type                4       /* control type: ring, callback function: (none) */
#define  PANEL_Set_ID                     5       /* control type: string, callback function: add_new_Sensor */
#define  PANEL_Com_Port                   6       /* control type: numeric, callback function: (none) */
#define  PANEL_Connect                    7       /* control type: command, callback function: MyConnect */
#define  PANEL_Randow_changes             8       /* control type: binary, callback function: MyRD */
#define  PANEL_Delta                      9       /* control type: numeric, callback function: (none) */
#define  PANEL_Interval                   10      /* control type: numeric, callback function: (none) */
#define  PANEL_TIMER                      11      /* control type: timer, callback function: MyTimer */
#define  PANEL_TIMER_RD                   12      /* control type: timer, callback function: MyRDTimer */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

#define  MENUBAR                          1
#define  MENUBAR_MENU1                    2
#define  MENUBAR_MENU1_Save_template      3       /* callback function: SaveTemplate */
#define  MENUBAR_MENU1_Load_template      4       /* callback function: LoadTemplate */
#define  MENUBAR_MENU1_exit               5       /* callback function: menu_exit */
#define  MENUBAR_CommandLog               6
#define  MENUBAR_CommandLog_CommandLog_open 7     /* callback function: OpenLog */
#define  MENUBAR_CommandLog_Export_command_log 8  /* callback function: Save_command_log */

#define  MonitorMen                       2
#define  MonitorMen_File                  2
#define  MonitorMen_File_ClearLog         3       /* callback function: MyClearLog */
#define  MonitorMen_File_Export           4       /* callback function: Save_command_log */
#define  MonitorMen_File_Close            5       /* callback function: Close_window */


     /* Callback Prototypes: */

int  CVICALLBACK add_new_Sensor(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK Close_window(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK LoadTemplate(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK menu_exit(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK MyClearLog(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK MyConnect(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK MyRD(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK MyRDTimer(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK MyTimer(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK OpenLog(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK QuitCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK Save_command_log(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK SaveTemplate(int menubar, int menuItem, void *callbackData, int panel);


#ifdef __cplusplus
    }
#endif
