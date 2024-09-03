#include <cvirte.h>	
#include "sensors.h" 
#define MAX_SIZE_OF_ID 10 
#define MAX_SECTIONS 11
#define DISTANT_BETWEEN_SECTION 40
#define DISTANT_BETWEEN_CTRL 90
#define DISTANT_FROM_LEFT 10
#define DISTANT_FROM_TOP 50


/************** Static Function Declarations **************/

/************** Global Variable Declarations **************/



/************** Global Function Declarations **************/



////////////////////////////////////////////////
////////////////Define Struct///////////////////
////////////////////////////////////////////////

DLLEXPORT typedef struct {
	int inx;
	char id_sensor[MAX_SIZE_OF_ID];
    int id_ctrl;
	int type_ctrl;
    int data_ctrl;
    int requestUpdate_ctrl;
} Section_ctrl;



DLLEXPORT typedef struct Section_Ctrl_Node{
	Section_ctrl *section;
    struct Section_Ctrl_Node *next;
} Section_Ctrl_Node;

DLLEXPORT typedef struct {
	int portnumber;
	Section_Ctrl_Node *sectionHead;
	Node *sensorHead;
} base_info;


/////////////////////////////////////////////

extern Section_ctrl* initializeNewSection(int panel,char *id,int inx,base_info *mainInfo);
extern int CVICALLBACK MyRequestUpdate (int panel, int control, int event,void *callbackData, int eventData1, int eventData2);
extern int CreateSections(int panel,Section_Ctrl_Node **sectionHead,base_info *mainInfo);
extern int UpdateSectionData(int panel,base_info *mainInfo,char * id,Section_Ctrl_Node *cur_section);
extern char *FindSectionIdByUpdateButton(base_info *mainInfo, int requestUpdate_ctrl);
extern int UpdateAllSectionData(int panel,base_info *mainInfo);
extern void freeCtrlSection(int panel,Section_Ctrl_Node *sectionHead);
