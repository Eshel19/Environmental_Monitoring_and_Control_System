#include <utility.h>
#include <rs232.h>
#include <ansi_c.h>
	
#include <userint.h>

#include "communication.h" 
#include "section_ctrl.h"


//////Function Define//////////////////

Section_ctrl* initializeNewSection(int panel,char *id,int inx,base_info *mainInfo);
int CreateSections(int panel,Section_Ctrl_Node **sectionHead,base_info *mainInfo);
int UpdateSectionData(int panel,base_info *mainInfo,char * id,Section_Ctrl_Node *cur_section);
char *FindSectionIdByUpdateButton(base_info *mainInfo, int requestUpdate_ctrl);
int UpdateAllSectionData(int panel,base_info *mainInfo);
void freeCtrlSection(int panel,Section_Ctrl_Node *sectionHead);
int CVICALLBACK MyRequestUpdate (int panel, int control, int event,void *callbackData, int eventData1, int eventData2);



//////////////////////////////////////


///////Global parameters/////////////////

static int numOfSections=0;
int (__cdecl *ptrMyRequestUpdate)(int, int, int, void *, int, int) =  MyRequestUpdate; 


////////////////////////////////////////


Section_ctrl* initializeNewSection(int panel,char *id,int inx,base_info *mainInfo) {
	if(inx>MAX_SECTIONS) return NULL;
	Node *sensorHead=mainInfo->sensorHead;
	Section_ctrl* newSection=malloc(sizeof(Section_ctrl));
	if (newSection == NULL) {
        return NULL;
    }
    strncpy(newSection->id_sensor, id, MAX_SIZE_OF_ID*sizeof(char));
    newSection->inx = inx;
    // Create controls dynamically
	
    newSection->id_ctrl = NewCtrl(panel, CTRL_STRING, "ID", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT);
    newSection->type_ctrl = NewCtrl(panel, CTRL_STRING, "Type", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT+DISTANT_BETWEEN_CTRL);
	newSection->data_ctrl = NewCtrl(panel, CTRL_NUMERIC, getSensorTypeUnit(getSensorType(sensorHead,id)), inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT+DISTANT_BETWEEN_CTRL*2);
    newSection->requestUpdate_ctrl = NewCtrl(panel, CTRL_SQUARE_COMMAND_BUTTON, "Request data", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT+DISTANT_BETWEEN_CTRL*3);
    
	//set control mode and other setting for each ctrl
	SetCtrlAttribute (panel,newSection->id_ctrl , ATTR_CTRL_MODE, 0);
	SetCtrlAttribute (panel,newSection->type_ctrl , ATTR_CTRL_MODE, 0);
	SetCtrlAttribute (panel,newSection->data_ctrl , ATTR_CTRL_MODE, 0);
	SensorType tmptype=getSensorType(sensorHead,id);
	char* type_str=getSensorTypeString(tmptype);
	SetCtrlVal (panel, newSection->type_ctrl, type_str);
	SetCtrlVal (panel, newSection->id_ctrl, id);
	SetCtrlAttribute (panel, newSection->requestUpdate_ctrl, ATTR_CALLBACK_FUNCTION_POINTER, ptrMyRequestUpdate);
	SetCtrlAttribute (panel, newSection->requestUpdate_ctrl, ATTR_CALLBACK_DATA, (void*)mainInfo);
	
	return newSection;
}

int CreateSections(int panel,Section_Ctrl_Node **sectionHead,base_info *mainInfo)
{
	Node *sensorHead=mainInfo->sensorHead;
	numOfSections=0;
	Section_Ctrl_Node *current = *sectionHead;
	Node *correctSensor=sensorHead;
	while(correctSensor!=NULL&&numOfSections<MAX_SECTIONS)
	{
		current->section=initializeNewSection(panel,correctSensor->sensor->id,numOfSections,mainInfo);
		if(correctSensor->next!=NULL)
		{
			current->next=(Section_Ctrl_Node*) malloc(sizeof(Section_Ctrl_Node));
			if(current->next==NULL) return -2;// error code for fail to allocat memory
			current=current->next;
		}else current->next=NULL;
		
		numOfSections++;
		correctSensor=correctSensor->next;
	}
	if(numOfSections==MAX_SECTIONS) return -1;
	return numOfSections;
}

void freeCtrlSection(int panel,Section_Ctrl_Node *sectionHead)
{
	if(sectionHead!=NULL)
	{
		freeCtrlSection(panel,sectionHead->next);
		DiscardCtrl (panel, sectionHead->section->id_ctrl);
		DiscardCtrl (panel, sectionHead->section->data_ctrl); 
		DiscardCtrl (panel, sectionHead->section->requestUpdate_ctrl); 
		DiscardCtrl (panel, sectionHead->section->type_ctrl);
		free(sectionHead->section);
		free(sectionHead);
	}else numOfSections=0;
}

int UpdateSectionData(int panel,base_info *mainInfo,char * id,Section_Ctrl_Node *cur_section)//if cur_section =NULL look for the correct section
{
	Node *sensorHead=mainInfo->sensorHead;
	Section_Ctrl_Node *sectionHead= mainInfo->sectionHead;
	
	double data=getSensorData(sensorHead,id);
	if(data==-1) return -2; // cannot find sensor for this id
	if(cur_section!=NULL)
	{
		SetCtrlVal(panel,cur_section->section->data_ctrl,data);	
	}
	else
	{
		Section_Ctrl_Node *current = sectionHead;
		while(current !=NULL&&(strcmp(current->section->id_sensor, id) != 0)) current=current->next;
		if(current==NULL) return -1;// cannot find section for this sensor
		SetCtrlVal(panel,current->section->data_ctrl,data);
	}
	return 0;
}

char *FindSectionIdByUpdateButton(base_info *mainInfo, int requestUpdate_ctrl) {
    
	Node *sensorHead=mainInfo->sensorHead;
	Section_Ctrl_Node *sectionHead= mainInfo->sectionHead;

	Section_Ctrl_Node *current =sectionHead;
    while (current != NULL) {
        if (current->section->requestUpdate_ctrl == requestUpdate_ctrl) {
            return current->section->id_sensor;
        }
        current = current->next;
    }
    return NULL; // Indicate that the section was not found
}

int UpdateAllSectionData(int panel,base_info *mainInfo)
{
	
	Node *sensorHead=mainInfo->sensorHead;
	Section_Ctrl_Node *sectionHead= mainInfo->sectionHead;
	Section_Ctrl_Node *current = sectionHead;
	int err=0;
	while(current!=NULL)
	{
		
		err=UpdateSectionData(panel,mainInfo,current->section->id_sensor,current);
		current=current->next;
	}
	return err;
	
}

int CVICALLBACK MyRequestUpdate (int panel, int control, int event,void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			base_info *mainInfo=(base_info*)callbackData;
			char *id=FindSectionIdByUpdateButton(mainInfo,control);
			if(id==NULL) return;
			char massage[SEND_BUFFER_SIZE];
			//massage[0]=Request_Sensor_Data;
			//massage[1]=0;
			sprintf(massage,"%c%s",Request_Sensor_Data,id);
			int a=strlen(massage);
			massage[a++]=0;
			massage[a++]='\r';
			ComWrt (mainInfo->portnumber, massage, a);
		break;
	}
	return 0;
}
