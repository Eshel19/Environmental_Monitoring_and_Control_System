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

#define  Connect                          1
#define  Connect_QUITBUTTON               2       /* control type: command, callback function: QuitCallback */
#define  Connect_Portnumber               3       /* control type: numeric, callback function: MyConnect */
#define  Connect_Connect                  4       /* control type: command, callback function: MyConnect */
#define  Connect_TIMER                    5       /* control type: timer, callback function: MyTimer */


     /* Control Arrays: */

          /* (no control arrays in the resource file) */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK MyConnect(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK MyTimer(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK QuitCallback(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
