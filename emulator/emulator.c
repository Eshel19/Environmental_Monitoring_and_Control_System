#include <utility.h>
#include <rs232.h>
#include <ansi_c.h>
#include <cvirte.h>	
#include <time.h>
#include <userint.h>
#include "emulator.h"
#include "sensors.h"
#include "communication.h"

#define MAX_SECTIONS 11
#define DISTANT_BETWEEN_SECTION 40
#define DISTANT_BETWEEN_CTRL 90
#define DISTANT_FROM_LEFT 10
#define DISTANT_FROM_TOP 50
#define MAX_LOG_SIZE 10000000
#define BUFFER_SIZE 200

typedef enum{
	offline,
	idle,
	sending_list,	//stata for sending list of connected sensors 
	sending_data, 	//stata for sending singel sensor data 
	streaming, //stata for streaming data accourding to the inteval 
} state;

//////////////////////Define Structs
typedef struct {
	int inx;
	char id_sensor[MAX_SIZE_OF_ID];
    int id_ctrl;
	int type_ctrl;
    int data_ctrl;
    int updateButton_ctrl;
    int removeButton_ctrl;
} Section;

typedef struct SectionNode{
	Section *section;
    struct SectionNode *next;
} SectionNode;


/////////////////////////////////



/////////Define global parametes 
static int panelHandle,number_of_section;
static int panelMonitorHandle=-1;
static Node* sensorHead;
static SectionNode* sectionHead;
static int ComPort=4;
int Err;
static int streaming_on;
static state currect_state=offline;
static int sensor_lock,sm_lock,log_lock;
static int threadFunctionId,rdThread;
static char sendLog[MAX_LOG_SIZE];
static char recieveLog[MAX_LOG_SIZE];
static int pollingMode =1;
static int rd_on=0;
////////////////////////////////

void insertToLog(char source[],char newMassage[]); 
void CVICALLBACK RecieveCallback(int portNumber, int eventMask, void *callbackData); 
Section* initializeNewSection(int panel,char *id,int inx); 
void RemoveSectionNode(int panel,SectionNode **sectionHead,Node **sensorhead, int index);
SectionNode* addSection(int panel,SectionNode *sectionHead, char *id,int inx);
int FindSectionIndexByUpdateButton(SectionNode *head, int updateButton_ctrl);
int FindSectionIndexByRemoveButton(SectionNode *head, int removeButton_ctrl);
int UpdateSensorFromSectionByInx(int panel, int inx,SectionNode *sectionhead,Node *sensorHead);
int CVICALLBACK MyRemove (int panel, int control, int event,void *callbackData, int eventData1, int eventData2);
int CVICALLBACK MyUpdate (int panel, int control, int event,void *callbackData, int eventData1, int eventData2);
int CVICALLBACK MyDisconnect (int panel, int control, int event,void *callbackData, int eventData1, int eventData2);
int CVICALLBACK MyConnect (int panel, int control, int event,void *callbackData, int eventData1, int eventData2);
static int streamingRun(void * intervales) ;
void smControl(state next_state,char massage[]) ;
void sendList();
void sendData(char massage[]);
void GetCurrentTimeAsString(char *timeStr);
static int RandChange(void *a);
void UpdateSectionData(SectionNode *sectionHead,int inx,double data) ;

int (__cdecl *ptrMyRemove)(int, int, int, void *, int, int) =  MyRemove;  
int (__cdecl *ptrMyUpdate)(int, int, int, void *, int, int) =  MyUpdate; 
int (__cdecl *ptrMyConnect)(int, int, int, void *, int, int) =  MyConnect;  
int (__cdecl *ptrMyDisconnect)(int, int, int, void *, int, int) =  MyDisconnect;  



///////////////////////////////////////////////////////////
//
//		All struct function for creating a section and managing it 
//
///////////////////////////////////////////////////////////


Section* initializeNewSection(int panel,char *id,int inx) {
	if(inx>MAX_SECTIONS) return NULL;
	Section* newSection=malloc(sizeof(Section));
	if (newSection == NULL) {
        return NULL;
    }
    strncpy(newSection->id_sensor, id, MAX_SIZE_OF_ID*sizeof(char));
    newSection->inx = inx;
    // Create controls dynamically
    newSection->id_ctrl = NewCtrl(panel, CTRL_STRING, "ID", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT);
    newSection->type_ctrl = NewCtrl(panel, CTRL_STRING, "Type", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT+DISTANT_BETWEEN_CTRL);
	newSection->data_ctrl = NewCtrl(panel, CTRL_NUMERIC, "data", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT+DISTANT_BETWEEN_CTRL*2);
    newSection->updateButton_ctrl = NewCtrl(panel, CTRL_SQUARE_COMMAND_BUTTON, "Update Value", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT+DISTANT_BETWEEN_CTRL*3);
    newSection->removeButton_ctrl = NewCtrl(panel, CTRL_SQUARE_COMMAND_BUTTON, "Remove", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT+DISTANT_BETWEEN_CTRL*4);
    
	//set control mode and other setting for each ctrl
	SetCtrlAttribute (panel,newSection->id_ctrl , ATTR_CTRL_MODE, 0);
	SetCtrlAttribute (panel,newSection->type_ctrl , ATTR_CTRL_MODE, 0);
	SensorType tmptype=getSensorType(sensorHead,id);
	char* type_str=getSensorTypeString(tmptype);
	SetCtrlVal (panel, newSection->type_ctrl, type_str);
	SetCtrlVal (panel, newSection->id_ctrl, id);
	SetCtrlAttribute (panel, newSection->updateButton_ctrl, ATTR_CALLBACK_FUNCTION_POINTER, ptrMyUpdate);
	SetCtrlAttribute (panel, newSection->removeButton_ctrl, ATTR_CALLBACK_FUNCTION_POINTER, ptrMyRemove);
	
	return newSection;
}

void RemoveSectionNode(int panel,SectionNode **sectionHead,Node **sensorhead, int index) {
    SectionNode *current = *sectionHead;
    SectionNode *previous = NULL;

    // Find the node to remove
    while (current != NULL && current->section->inx != index) {
        previous = current;
        current = current->next;
    }

    // If the node was not found, return
    if (current == NULL) {
        return;
    }

    // Remove the node
    if (previous == NULL) {
        // If the node to remove is the first node, update the head pointer
        *sectionHead = current->next;
    } else {
        // Otherwise, update the previous node's next pointer
        previous->next = current->next;
    }

    //Discard the ctrls
	DiscardCtrl(panel,current->section->id_ctrl);
    DiscardCtrl(panel, current->section->type_ctrl);
    DiscardCtrl(panel, current->section->data_ctrl);
    DiscardCtrl(panel, current->section->updateButton_ctrl);	
	DiscardCtrl(panel, current->section->removeButton_ctrl);
	CmtGetLock (sensor_lock);
	removeSensor(sensorhead,current->section->id_sensor);
	CmtReleaseLock (sensor_lock);
	
	// Free the memory allocated for the node 
    free(current->section);
    free(current);
	current=NULL;

    // Update the inx field of the remaining sections
    current = *sectionHead;
    int newIndex = 0;
    while (current != NULL) {
        current->section->inx = newIndex;
        newIndex++;
        current = current->next;
    }
	current=*sectionHead;
	while(current!=NULL) 
	{
        SetCtrlAttribute(panel, current->section->id_ctrl, ATTR_TOP, ((current->section->inx) * DISTANT_BETWEEN_SECTION)+DISTANT_FROM_TOP);
        SetCtrlAttribute(panel, current->section->type_ctrl, ATTR_TOP, ((current->section->inx) * DISTANT_BETWEEN_SECTION)+DISTANT_FROM_TOP);
        SetCtrlAttribute(panel, current->section->data_ctrl, ATTR_TOP, ((current->section->inx) * DISTANT_BETWEEN_SECTION)+DISTANT_FROM_TOP);
        SetCtrlAttribute(panel, current->section->updateButton_ctrl, ATTR_TOP, ((current->section->inx) * DISTANT_BETWEEN_SECTION)+DISTANT_FROM_TOP);
		SetCtrlAttribute(panel, current->section->removeButton_ctrl, ATTR_TOP, ((current->section->inx) * DISTANT_BETWEEN_SECTION)+DISTANT_FROM_TOP);
		current=current->next;
	}
	
}

SectionNode* addSection(int panel,SectionNode *sectionHead, char *id,int inx) {
	SectionNode *newNodeSection = (SectionNode*) malloc(sizeof(SectionNode));
	
	if(newNodeSection==NULL) return NULL; //Cannot allocate memory for new new Node Section
	newNodeSection->section=initializeNewSection(panel,id, inx);
	if(newNodeSection->section==NULL) return NULL; //Cannot allocate memory for new Section 
	newNodeSection->next=NULL;
	if(sectionHead==NULL) sectionHead=newNodeSection; 	
	else
	{
		SectionNode *tmpNode=sectionHead;
		while(tmpNode->next!=NULL) tmpNode=tmpNode->next; 
		tmpNode->next=newNodeSection;
		
	}
    return sectionHead;
}

int FindSectionIndexByUpdateButton(SectionNode *head, int updateButton_ctrl) {
    SectionNode *current = head;
    while (current != NULL) {
        if (current->section->updateButton_ctrl == updateButton_ctrl) {
            return current->section->inx;
        }
        current = current->next;
    }
    return -1; // Indicate that the section was not found
}

int UpdateSensorFromSectionByInx(int panel, int inx,SectionNode *sectionhead,Node *sensorHead) {
    SectionNode *current = sectionhead;
    while ((current != NULL)&&current->section->inx != inx) {
        current = current->next;
    }
	if(current==NULL) return -1; // Indicate that the section was not found
	char tmpId[MAX_SIZE_OF_ID];
	strcpy(tmpId,current->section->id_sensor);
	double data;
	GetCtrlVal (panel,current->section->data_ctrl , &data);
	CmtGetLock (sensor_lock);
	setSensorData(sensorHead,tmpId,data);
	CmtReleaseLock (sensor_lock);
	
	return 0;
}

int FindSectionIndexByRemoveButton(SectionNode *head, int removeButton_ctrl) {
    SectionNode *current = head;
    while (current != NULL) {
        if (current->section->removeButton_ctrl == removeButton_ctrl) {
            return current->section->inx;
        }
        current = current->next;
    }
    return -1; // Indicate that the section was not found
}

void freeSections(int panel,SectionNode *head)
{
	if(head!=NULL)
	{
		freeSections(panel,head->next);
		DiscardCtrl(panel,head->section->id_ctrl);
    	DiscardCtrl(panel, head->section->type_ctrl);
    	DiscardCtrl(panel, head->section->data_ctrl);
    	DiscardCtrl(panel, head->section->updateButton_ctrl);	
		DiscardCtrl(panel, head->section->removeButton_ctrl);
		free(head->section);
		free(head);
	}else number_of_section =0;
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////

    


int main (int argc, char *argv[])
{
	if (InitCVIRTE (0, argv, 0) == 0)
		return -1;	/* out of memory */
	if ((panelHandle = LoadPanel (0, "emulator.uir", PANEL)) < 0)
		return -1;

	int i;
	for(i=0;i<NUMBER_OF_SENSORS_TYPES;i++)
	{
		InsertListItem (panelHandle, PANEL_Sensor_type, i, getSensorTypeString(i), i);
	}
	CmtNewLock (NULL, 0, &sensor_lock);
	CmtNewLock (NULL, 0, &sm_lock);
	CmtNewLock (NULL, 0, &log_lock);
	sensorHead=NULL;
	sectionHead=NULL;
	DisplayPanel (panelHandle);
	RunUserInterface ();
	freeSections(panelHandle,sectionHead);
	DiscardPanel (panelHandle);
	
	int Err = CloseCom (ComPort); 
	freelist(sensorHead);
	
	return 0;
}


int CVICALLBACK QuitCallback (int panel, int control, int event,
							  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			QuitUserInterface (0);
			break;
	}
	return 0;
}

int CVICALLBACK add_new_Sensor (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			char tmpId[MAX_SIZE_OF_ID];
			int tmp;
			SensorType tmpType;
			GetCtrlVal (panel, PANEL_Set_ID, tmpId);
			GetCtrlVal (panel, PANEL_Sensor_type, &tmp);
			if((findSensor(sensorHead, tmpId)==NULL)&&number_of_section<MAX_SECTIONS&&tmpId[0]!=0)
			{
				tmpType=(SensorType)tmp;
				sensorHead=addSensor(sensorHead,tmpId,tmpType); 
				sectionHead=addSection(panel,sectionHead,tmpId,number_of_section);
				number_of_section++;
			}
			
			break;
	}
	return 0;
}


int CVICALLBACK MyRemove (int panel, int control, int event,
						 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int in=FindSectionIndexByRemoveButton(sectionHead,control);
			RemoveSectionNode(panel,&sectionHead,&sensorHead,in);
			number_of_section--;
			break;
	}
	return 0;
}

int CVICALLBACK MyUpdate (int panel, int control, int event,
						 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			
			int inx=FindSectionIndexByUpdateButton(sectionHead,control);
			UpdateSensorFromSectionByInx(panel, inx,sectionHead,sensorHead);
			
			break;
	}
	return 0;
}



void CVICALLBACK SaveTemplate (int menuBar, int menuItem, void *callbackData,
							   int panel)
{
	if(sectionHead==NULL)
	{
		MessagePopup("Error", "Sensor list is empty!");
		return;
	}
	char pathName[MAX_PATHNAME_LEN];
	if (FileSelectPopup("", "*.txt", "*.txt", "Save File", VAL_SAVE_BUTTON, 0, 0, 1, 1, pathName))
    {
		FILE *fp = fopen(pathName, "w");
		Node* tmp=sensorHead;
		char *id;
		double data;
		SensorType type;
		while(tmp!=NULL)
		{
			id=tmp->sensor->id;
			type=tmp->sensor->type;  
			data=tmp->sensor->data;
			fprintf(fp,"%s ,%d,% .3f\n",id,type,data);
			
			tmp=tmp->next;
		}
		fclose(fp);
	}
	
}

void CVICALLBACK LoadTemplate (int menuBar, int menuItem, void *callbackData,
							   int panel)
{
	CmtGetLock(sm_lock);
	if(currect_state==streaming)
	{
		MessagePopup("Error!", "The emolator is streaming data this oparetion cannot be complate at this time.");
		CmtReleaseLock(sm_lock);
		return;
	}
	CmtReleaseLock(sm_lock);	
	if(sectionHead!=NULL)
	{
		int pick=ConfirmPopup ("Warrning!", "This action will remove all the existing sensors\nWould you still like to contiunue?");
		if(pick==0)
			return;
	}
	freeSections(panel,sectionHead);
	freelist(sensorHead);
	sectionHead=NULL;
	sensorHead=NULL;
	char pathName[MAX_PATHNAME_LEN];
	if (FileSelectPopup("", "*.txt", "*.txt", "Load File", VAL_LOAD_BUTTON, 0, 0, 1, 1, pathName))
    {
		FILE *fp = fopen(pathName, "r");
		char *id;
		char *dataStr;
		double data;
		SensorType type;
		char *typeStr;
		char *endptr;
		char buffer[BUFFER_SIZE];
		number_of_section=0;
		while(fgets(buffer, BUFFER_SIZE, fp))
		{
			
			
			id = strtok(buffer, ",");
			id[strcspn(id, " ,\n")] = 0;
			
			
			typeStr=strtok(NULL, ","); 
			typeStr[strcspn(typeStr, " ,\n")] = 0;
			type=(SensorType)atoi(typeStr);
			
			dataStr = strtok(NULL, ","); 
			//dataStr[strcspn(dataStr, " ,\n")] = 0;
			data= strtod(dataStr, &endptr);
			
			if((findSensor(sensorHead, id)==NULL)&&number_of_section<MAX_SECTIONS&&id[0]!=0)
			{
				sensorHead=addSensor(sensorHead,id,type); 
				sectionHead=addSection(panelHandle,sectionHead,id,number_of_section);
				UpdateSectionData(sectionHead,number_of_section,data);
				UpdateSensorFromSectionByInx(panel, number_of_section,sectionHead,sensorHead);
				
				number_of_section++;
			}
			
		}
		fclose(fp);
	}
	
	
}

void CVICALLBACK menu_exit (int menuBar, int menuItem, void *callbackData,
							int panel)
{
	QuitUserInterface (0);	
}

void CVICALLBACK OpenLog (int menuBar, int menuItem, void *callbackData,
						  int panel)
{
	if(panelMonitorHandle!=-1) return;
	
	
	if ((panelMonitorHandle = LoadPanel (0, "emulator.uir", MONITOR)) < 0)
		return;
		DisplayPanel (panelMonitorHandle);
		ResetTextBox (panelMonitorHandle,MONITOR_SendMSG , "");
		ResetTextBox (panelMonitorHandle,MONITOR_RecieveMSG , ""); 
		InsertTextBoxLine (panelMonitorHandle, MONITOR_SendMSG, 0, sendLog); 
		InsertTextBoxLine (panelMonitorHandle, MONITOR_RecieveMSG, 0, recieveLog); 
		
		
}


void WriteLogToFile(const char* log, const char* filename) {
	
	if(log[0]=='\0')
	{
		MessagePopup("Error", "Log data in empty");
		return;
	}
	char pathName[MAX_PATHNAME_LEN];
	char title[MAX_PATHNAME_LEN];
	sprintf(title,"Save %s file",filename);
	if (FileSelectPopup("", "*.txt", "*.txt", title, VAL_SAVE_BUTTON, 0, 0, 1, 1, pathName))
	{
	    FILE *file = fopen(pathName, "w");
	    if (file != NULL) {
	        for (int i = 0; log[i] != '\0' && i < MAX_LOG_SIZE; ++i) {
	            fputc(log[i], file);
	        }
	        fclose(file);
	    } else {
	        MessagePopup("Error", "File opening failed");
	    }
	}
}

void CVICALLBACK Save_command_log (int menuBar, int menuItem, void *callbackData,
								   int panel)
{

	CmtGetLock(log_lock); 
	WriteLogToFile(sendLog, "send log");
    WriteLogToFile(recieveLog, "recieve log");
	CmtReleaseLock(log_lock); 
	
}

int CVICALLBACK MyConnect (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal (panel, PANEL_Com_Port, &ComPort);
			int Err = OpenComConfig (ComPort, "", Baud_Rate, Parity, Data_Bits, Stop_Bits, SEND_BUFFER_SIZE, SEND_BUFFER_SIZE);
			SetComTime (ComPort, 0.01);
			FlushInQ (ComPort);
			FlushOutQ (ComPort);
			if(pollingMode)
			{
					SetCtrlAttribute (panel, PANEL_TIMER, ATTR_ENABLED, 1);

			}
			else
			{
				InstallComCallback (ComPort, LWRS_RXFLAG, 0, 0, RecieveCallback, 0);
			}
			
			SetCtrlAttribute (panel, control, ATTR_CALLBACK_FUNCTION_POINTER, ptrMyDisconnect);
			SetCtrlAttribute (panel, control, ATTR_LABEL_TEXT, "Disconnect");
			smControl(idle,NULL);
			
			
			break;
	}
	return 0;
}

int CVICALLBACK MyDisconnect (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			smControl(offline,NULL);
			int Err = CloseCom (ComPort);
			SetCtrlAttribute (panel, control, ATTR_CALLBACK_FUNCTION_POINTER, ptrMyConnect);
			SetCtrlAttribute (panel, control, ATTR_LABEL_TEXT, "Connect");
			if(pollingMode)
			{
					SetCtrlAttribute (panel, PANEL_TIMER, ATTR_ENABLED, 0);

			}
			

			break;
	}
	return 0;
}

void CVICALLBACK RecieveCallback(int portNumber, int eventMask, void *callbackData)
{
	
	char RecChar[2];
	char massage[SEND_BUFFER_SIZE];

	ComRd (ComPort, RecChar, 1);
	char commade=RecChar[0];
	
	ComRd (ComPort, massage, SEND_BUFFER_SIZE-1);
	switch(commade)
	{
		case Request_to_List_Sensors:
			smControl(sending_list,NULL);
			smControl(idle,NULL);
		break;
		case Request_Sensor_Data:
			smControl(sending_data,massage);
			smControl(idle,NULL);
		break;
		case Start_Data_Stream:
			smControl(streaming,massage);
		break;
		case Stop_Data_Stream:
			smControl(idle,NULL);
		break;
		default: return ;
	}
	char tmp[100];
	sprintf(tmp,"%c%s" ,commade,massage);
	insertToLog(recieveLog,tmp);
		
	return ;
}
void insertToLog(char source[],char newMassage[])
{
	char currentTimeStr[50];
	GetCurrentTimeAsString(currentTimeStr);
	sprintf(source,  "%s%s> %s\n",source,currentTimeStr,newMassage);
	if(panelMonitorHandle!=-1) 
	{
		ResetTextBox (panelMonitorHandle,MONITOR_SendMSG , "");
		InsertTextBoxLine (panelMonitorHandle, MONITOR_SendMSG, 0, sendLog);
		ResetTextBox (panelMonitorHandle,MONITOR_RecieveMSG , "");
		InsertTextBoxLine (panelMonitorHandle, MONITOR_RecieveMSG, 0, recieveLog);
	}
}

void CVICALLBACK Close_window (int menuBar, int menuItem, void *callbackData,
							   int panel)
{
	if(panel!=-1) 
	{
		DiscardPanel (panel);
		if(panel==panelMonitorHandle) 
		{
			panelMonitorHandle=-1;
			return;
		}
	}
	
}
void smControl(state next_state,char massage[])
{
	switch(currect_state)
	{
		case offline:
			if (next_state==idle) currect_state=idle; 
		break;
		case idle:
			if(next_state==sending_list)
			{
				currect_state=sending_list;
				sendList();
			}
			if(next_state==sending_data)
			{
				currect_state=sending_data;
				sendData(massage);
			}
			if(next_state==streaming)
			{
				CmtGetLock (sm_lock);
				streaming_on=1;
				CmtReleaseLock (sm_lock);
				CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE, streamingRun, ((double *)massage), &threadFunctionId);
				currect_state=streaming;
			}
			if(next_state==offline) currect_state=offline;
		break;
		case sending_list:
			if (next_state==idle) currect_state=idle;
			if (next_state==offline) currect_state=offline;
		break;
		case sending_data:
			if (next_state==idle) currect_state=idle;
			if (next_state==offline) currect_state=offline;
		break;
		case streaming:
			
			if (next_state==idle||next_state==offline) 
			{
				CmtGetLock (sm_lock);
				streaming_on=0;
				CmtReleaseLock (sm_lock);
				CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE, threadFunctionId, OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
				CmtReleaseThreadPoolFunctionID (DEFAULT_THREAD_POOL_HANDLE, threadFunctionId);
				currect_state=next_state;
			}
		break;
		
		
	}
}

void sendData(char massage[]){
	double data=getSensorData(sensorHead,massage);
	char buffer[SEND_BUFFER_SIZE];
	char tmplog[SEND_BUFFER_SIZE];
	tmplog[0]=0;
	sprintf(tmplog,"0xA2%s%.2f",massage,data);  
	int i=1;
	sprintf(buffer,"%c%s",Sensor_Data_Response,massage);
	i+=strlen(buffer);
	memcpy(buffer+i, &data, sizeof(double));
	i+=sizeof(double);
	insertToLog(sendLog,tmplog);
	buffer[i++]='\r';
	ComWrt (ComPort, buffer, i);
}


void sendList()
{
	char buffer[SEND_BUFFER_SIZE];
	char tmplog[SEND_BUFFER_SIZE];
	char tmplog2[SEND_BUFFER_SIZE];  
	buffer[0]=List_of_Sensors_Response;
	int i=1;
	Node* tmp = sensorHead;
	CmtGetLock(sensor_lock);
	while(tmp!=NULL&&i<(SEND_BUFFER_SIZE-1))
	{
		int j=0;
		
		while(tmp->sensor->id[j]!=0&&j<MAX_SIZE_OF_ID&&(i+j)<(SEND_BUFFER_SIZE-1)) 
		{
		buffer[i+j]=tmp->sensor->id[j];
		
			j++;
		}
		i+=j;
		sprintf(tmplog2,"%s%s%s",tmplog,tmp->sensor->id,getSensorTypeString(tmp->sensor->type)); 
		buffer[i++]=0;
		strcpy(tmplog,tmplog2);
		if(i<(SEND_BUFFER_SIZE-1)) buffer[i++]=(char)tmp->sensor->type;
		tmp=tmp->next;;
	}
	CmtReleaseLock(sensor_lock);  
	if(i>=(SEND_BUFFER_SIZE))
	{
		buffer[SEND_BUFFER_SIZE-1]='\r';
		ComWrt (ComPort, buffer, SEND_BUFFER_SIZE);
	}
	else
	{
		buffer[i++]='\r';
		ComWrt (ComPort, buffer, i);
	}
	sprintf(tmplog2,"0xA1%s",tmplog);
	insertToLog(sendLog,tmplog2);   
	strcpy(tmplog2,"");
	strcpy(tmplog,"");
	
	
}

static int streamingRun(void *intervales)
{
	double t=0;
	memcpy(&t,intervales,sizeof(double));
	CmtGetLock (sm_lock);
	int a=streaming_on;
	CmtReleaseLock (sm_lock);
	char buffer[SEND_BUFFER_SIZE];
	char tmplog[SEND_BUFFER_SIZE];
	char tmplog2[SEND_BUFFER_SIZE];
	buffer[0]=Data_Stream_Response;
	
	while(a)
	{
		buffer[0]=Data_Stream_Response;
		int i=1;
		int k=0;
		Node* tmp = sensorHead;
		while(tmp!=NULL&&i<(SEND_BUFFER_SIZE-1))
		{
			 
			int j=0;
			while(tmp->sensor->id[j]!=0&&j<MAX_SIZE_OF_ID&&(i+j)<SEND_BUFFER_SIZE&&(i+j)<(SEND_BUFFER_SIZE-1)) 
			{
				buffer[i+j]=tmp->sensor->id[j];
				j++;
			}
			i+=j;
			 
			if(i>=(SEND_BUFFER_SIZE)) break;
			tmplog[k+j]=0;
			buffer[i++]=0;
			
			if((i+sizeof(double))<(SEND_BUFFER_SIZE-1)) 
			{
				
				CmtGetLock (sensor_lock);
				double data =tmp->sensor->data;	
				CmtReleaseLock (sensor_lock);

				memcpy(buffer+i, &data, sizeof(double));
				sprintf(tmplog2,"%s%s;%.2f",tmplog,tmp->sensor->id,data);
				k+=strlen(tmplog2);
				strcpy(tmplog,tmplog2);
				strcpy(tmplog2,"");
				i+=sizeof(double);
				
			}
			
			tmplog[k]=0;
			tmp=tmp->next;
		}
		if((i+sizeof(double))>=(SEND_BUFFER_SIZE)) 
		{
			tmplog[SEND_BUFFER_SIZE-1]='\r';
			buffer[SEND_BUFFER_SIZE-1]='\r';
			ComWrt (ComPort, buffer, SEND_BUFFER_SIZE);
		}
		else
		{
			tmplog[i-1]='\r';
			buffer[i++]='\r';
			ComWrt (ComPort, buffer, i);
		}
		tmplog[k]=0;
		sprintf(tmplog2,"0xA3%s",tmplog);
		CmtGetLock(log_lock); 
		insertToLog(sendLog,tmplog2);
		CmtReleaseLock(log_lock); 
		strcpy(tmplog,"");
		strcpy(tmplog2,""); 
		Delay(t);
		
		CmtGetLock (sm_lock);
		a=streaming_on;
		CmtReleaseLock (sm_lock);
	}
	
	return 0;	
}



int CVICALLBACK MyTimer (int panel, int control, int event,
						 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_TIMER_TICK:
			
			
			
			char RecChar[SEND_BUFFER_SIZE];
			unsigned char commade;
			char *massage;
			ComRdTerm (ComPort, RecChar, SEND_BUFFER_SIZE,'\r');
			commade=RecChar[0];
			massage=RecChar+1;
			char tmp[100];
			switch(commade)
			{
				case Request_to_List_Sensors:
					sprintf(tmp,"0x01");
					smControl(sending_list,NULL);
					smControl(idle,NULL);
				break;
				case Request_Sensor_Data:
					sprintf(tmp,"0x02%s", massage); 
					smControl(sending_data,massage);
					smControl(idle,NULL);
				break;
				case Start_Data_Stream:

					sprintf(tmp,"0x03%.2f", ((double *)massage)[0]); 
					smControl(streaming,massage);
				break;
				case Stop_Data_Stream:
					sprintf(tmp,"0x04%s", massage); 
					smControl(idle,NULL);
				break;
				default: return 0;
			}
			insertToLog(recieveLog,tmp);
			RecChar[0]=0;
			RecChar[1]='\r';
			strcpy(tmp,"");
			break;
	}
	return 0;
}

void CVICALLBACK MyClearLog (int menuBar, int menuItem, void *callbackData,
							 int panel)
{
	
	strcpy(recieveLog,"");
	strcpy(sendLog,"");
	ResetTextBox (panelMonitorHandle,MONITOR_SendMSG , "");
	InsertTextBoxLine (panelMonitorHandle, MONITOR_SendMSG, 0, sendLog);
	ResetTextBox (panelMonitorHandle,MONITOR_RecieveMSG , "");
	InsertTextBoxLine (panelMonitorHandle, MONITOR_RecieveMSG, 0, recieveLog); 
}

void GetCurrentTimeAsString(char *timeStr) {
    time_t currentTime;
    struct tm *localTime;
    time(&currentTime);

    localTime = localtime(&currentTime);

    strftime(timeStr, 50, "%c", localTime);
}

int CVICALLBACK MyRD (int panel, int control, int event,
					  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int pick;
			double tr;
			double delt;
			GetCtrlVal(panel,PANEL_Randow_changes,&pick);
			GetCtrlVal(panel,PANEL_Interval,&tr); 
			SectionNode *tmp=sectionHead;
			
			while(tmp!=NULL)
			{
				SetCtrlAttribute (panel,tmp->section->updateButton_ctrl , ATTR_CTRL_MODE, !pick);
				SetCtrlAttribute (panel,tmp->section->removeButton_ctrl , ATTR_CTRL_MODE, !pick);
				SetCtrlAttribute (panel,tmp->section->data_ctrl , ATTR_CTRL_MODE, !pick);
				
				tmp=tmp->next;	
			}
			SetCtrlAttribute (panel, PANEL_TIMER_RD, ATTR_INTERVAL, tr);
			SetCtrlAttribute (panel,PANEL_Interval , ATTR_CTRL_MODE, !pick);
			SetCtrlAttribute (panel,PANEL_TIMER_RD , ATTR_ENABLED, pick); 
			

			
	}
	return 0;
}
int CVICALLBACK MyRDTimer (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_TIMER_TICK:
			double tr;
			double delt;
			GetCtrlVal (panelHandle, PANEL_Interval, &tr);
			GetCtrlVal (panelHandle, PANEL_Delta, &delt);
			int max=number_of_section;
			SectionNode *tmpHead=sectionHead;
			SectionNode *tmp=sectionHead;
			CmtGetLock(sensor_lock);
			Node * tmpSesHead=sensorHead;
			CmtReleaseLock(sensor_lock);
		
				while(tmp!=NULL)
				{
					CmtGetLock(sensor_lock);
					double addVal=getSensorData(tmpSesHead,tmp->section->id_sensor);
					CmtReleaseLock(sensor_lock);
					addVal=(double)rand() / RAND_MAX * (2 * delt) + (addVal - delt);
					SetCtrlVal(panelHandle,tmp->section->data_ctrl,addVal);
					UpdateSensorFromSectionByInx(panelHandle,tmp->section->inx,tmpHead,tmpSesHead);
					tmp=tmp->next;
			
				}
			break;
	}
	return 0;
}

void UpdateSectionData(SectionNode *sectionHead,int inx,double data)
{
	SectionNode *tmp=sectionHead;
	while(tmp!=NULL)
	{
		if(tmp->section->inx==inx)
		{
			SetCtrlVal(panelHandle,tmp->section->data_ctrl,data);
			return;
		}
		tmp=tmp->next;
	}
}
