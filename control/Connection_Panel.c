#include <rs232.h>
#include <cvirte.h>		
#include <userint.h>
#include <cvirte.h>
#include <utility.h>
#include "Connection_Panel.h"
#include "Monitor_panel.h"
#include "communication.h"
#include "section_ctrl.h"  

#define MAX_LOG_LENGHT 100000
#define BUFFER_SIZE 200

typedef enum{
	Nothing,
	waitForList,
	waitForMassage,
} Task;



void UpdateSensorsSections(char massage[],int l);
void UpdateSensorsData(char massage[],int l);
int SendReqForSensors(); 
int CVICALLBACK MyStopStreaming (int panel, int control, int event,void *callbackData, int eventData1, int eventData2);
int CVICALLBACK MyStartStreaming (int panel, int control, int event,void *callbackData, int eventData1, int eventData2);
void ResetWatchDog();
void StopWatchDog();
void StartWatchDog(Task task);
static int WatchDog(void *task);
void AlertManager(Task task);
void GetCurrentTimeAsString(char *timeStr);
void CVICALLBACK MyStopLog (int menuBar, int menuItem, void *callbackData,int panel);
void CVICALLBACK MyStartLog (int menuBar, int menuItem, void *callbackData,int panel);
static int Loging(void *intervals);


int Err;
static int panelConnect,panelMonitor;
static base_info* mainInfo;
static int log_lock,wg_lock,threadChecker,log_ch_lock,sensor_lock,logChecker;
static int WatchDogThread,LogingThread;
static int watchDogCounter;
int (__cdecl *ptrMyStopStreaming)(int, int, int, void *, int, int) =  MyStopStreaming; 
int (__cdecl *ptrMyStartStreaming)(int, int, int, void *, int, int) =  MyStartStreaming; 
static Task RetVal; 
static char dataLog[MAX_LOG_LENGHT];
static double t_t; 
static int threadID; 

int main (int argc, char *argv[])
{
	if (InitCVIRTE (0, argv, 0) == 0)
		return -1;	/* out of memory */
	if ((panelConnect = LoadPanel (0, "Connection_Panel.uir", Connect)) < 0)
		return -1;

	CmtNewLock (NULL, 0, &wg_lock);
	CmtNewLock (NULL, 0, &log_lock);
	CmtNewLock (NULL, 0, &log_ch_lock);
	CmtNewLock (NULL, 0, &sensor_lock); 
	DisplayPanel (panelConnect);
	panelMonitor=-1;
	RunUserInterface ();
	DiscardPanel (panelConnect);
	if(mainInfo!=NULL)
		 Err = CloseCom (mainInfo->portnumber);
	

	free(mainInfo);
	return 0;
}

int CVICALLBACK QuitCallback (int panel, int control, int event,
							  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			if(panelMonitor!=-1)
			{	
				freeCtrlSection(panelMonitor,mainInfo->sectionHead);
				CmtGetLock(sensor_lock); 
				freelist(mainInfo->sensorHead);
				CmtReleaseLock(sensor_lock);
			}
			QuitUserInterface (0);
			
			break;
	}
	return 0;
}

int CVICALLBACK MyConnect (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			int portnumber;
			GetCtrlVal (panel, Connect_Portnumber, &portnumber);
				mainInfo=malloc(sizeof(base_info));  
			mainInfo->sensorHead=NULL;
			mainInfo->sectionHead=malloc(sizeof(Section_Ctrl_Node));
			mainInfo->portnumber=portnumber;
			
			int Err = OpenComConfig (mainInfo->portnumber, "", Baud_Rate, Parity, Data_Bits, Stop_Bits, SEND_BUFFER_SIZE, SEND_BUFFER_SIZE);
			SetComTime (mainInfo->portnumber, 0.01);
			FlushInQ (mainInfo->portnumber);
			FlushOutQ (mainInfo->portnumber);
			SendReqForSensors();
			SetCtrlAttribute (panel,Connect_TIMER, ATTR_ENABLED, 1); 
			
			break;
	}
	return 0;
}

int CVICALLBACK MyOpenLog (int panel, int control, int event,
						   void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:

			break;
	}
	return 0;
}

int SendReqForSensors()
{
	char msg[2];
	msg[0]=Request_to_List_Sensors;
	msg[1]='\r';
	ComWrt (mainInfo->portnumber, msg, 2);
	StartWatchDog(waitForList);
	return 0;
}



int CVICALLBACK MyStartStreaming (int panel, int control, int event,
								  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:

			GetCtrlVal(panel,Monitor_Intervals,&t_t);
			char msg[20];
			msg[0]=Start_Data_Stream;
			memcpy(msg+1, &t_t, sizeof(double)); 
			msg[sizeof(double)+1]='\r';
			ComWrt (mainInfo->portnumber, msg, sizeof(double)+2);
			SetCtrlAttribute (panel, control, ATTR_CALLBACK_FUNCTION_POINTER, ptrMyStopStreaming);
			SetCtrlAttribute (panel, control, ATTR_LABEL_TEXT, "Stop streaming");
			StartWatchDog(waitForMassage);
			break;
	}
	return 0;
}

int CVICALLBACK MyStopStreaming (int panel, int control, int event,
								  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			
			char msg[2];
			msg[0]=Stop_Data_Stream; 
			msg[1]='\r';
			ComWrt (mainInfo->portnumber, msg, 2);
			SetCtrlAttribute (panel, control, ATTR_CALLBACK_FUNCTION_POINTER, ptrMyStartStreaming);
			SetCtrlAttribute (panel, control, ATTR_LABEL_TEXT, "Start streaming");
			StopWatchDog();
			break;
	}
	return 0;
}

void CVICALLBACK MyDisconnect (int menuBar, int menuItem, void *callbackData,int panel)
{
	
	char status[100];
	SetCtrlAttribute (panel, Monitor_StartStreaming, ATTR_LABEL_TEXT, status);
	if(strcmp(status,"Stop streaming")==0)
	{
		
		char msg[2];
		msg[0]=Stop_Data_Stream; 
		msg[1]='\r';
		ComWrt (mainInfo->portnumber, msg, 2);
		StopWatchDog(); 
	}
	
	if(panelMonitor!=-1)
	{
		freeCtrlSection(panelMonitor,mainInfo->sectionHead);
		CmtGetLock(sensor_lock); 
		freelist(mainInfo->sensorHead);
		CmtReleaseLock(sensor_lock);
		DiscardPanel(panelMonitor);
		DisplayPanel(panelConnect);
		panelMonitor=-1;
	}
	
}

void CVICALLBACK MyRefresh (int menuBar, int menuItem, void *callbackData,
							   int panel)
{
	
	char status[100];
	GetCtrlAttribute (panel, Monitor_StartStreaming, ATTR_LABEL_TEXT, status);
	if(strcmp(status,"Stop streaming")==0)
	{
		char msg[2];
		msg[0]=Stop_Data_Stream; 
		msg[1]='\r';
		ComWrt (mainInfo->portnumber, msg, 2);
		StopWatchDog();
		
	}
	StartWatchDog(waitForList);
	HidePanel(panelMonitor);
	freeCtrlSection(panelMonitor,mainInfo->sectionHead);
	CmtGetLock(sensor_lock); 
	freelist(mainInfo->sensorHead);
	CmtReleaseLock(sensor_lock);
	mainInfo->sensorHead=NULL;
	mainInfo->sectionHead=malloc(sizeof(Section_Ctrl_Node));
	SendReqForSensors();
	 

}


void CVICALLBACK MyMenuQuit (int menuBar, int menuItem, void *callbackData,
							   int panel)
{
	freeCtrlSection(panelMonitor,mainInfo->sectionHead);
	CmtGetLock(sensor_lock); 
	freelist(mainInfo->sensorHead);
	CmtReleaseLock(sensor_lock);
	QuitUserInterface (0);
}

void CVICALLBACK MyExportLog (int menuBar, int menuItem, void *callbackData,
							  int panel)
{
	CmtGetLock(log_lock);
	CmtGetLock(log_ch_lock);
	if(logChecker)
	{
		if(!ConfirmPopup ("Warrning!", "To export the log data the loging process needed to be stop\nare you sure you want to continue?"))
		{
			CmtReleaseLock(log_lock);
			CmtReleaseLock(log_ch_lock);
			return;
		}
		logChecker=0;
		CmtReleaseLock(log_ch_lock);
		CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE, LogingThread, OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
		SetMenuBarAttribute (menuBar,MENUBAR_Log_Start_Loging , ATTR_CALLBACK_FUNCTION_POINTER, MyStartLog);
		SetMenuBarAttribute (menuBar,MENUBAR_Log_Start_Loging , ATTR_ITEM_NAME, "Start loging");
	}
	else CmtReleaseLock(log_ch_lock);
	
	if(dataLog[0]=='\0')
	{
		MessagePopup ("Error", "Log data is empty cannot be export");
		CmtReleaseLock(log_lock);
		return;
	}
	char pathName[MAX_PATHNAME_LEN];
	if (FileSelectPopup("", "*.csv", "*.csv", "Save File", VAL_SAVE_BUTTON, 0, 0, 1, 1, pathName))
    {
		FILE *fp = fopen(pathName, "w");
		fprintf(fp,"%s",dataLog);
		fclose(fp);
	}
	CmtReleaseLock(log_lock);
	
}
void CVICALLBACK MyStartLog (int menuBar, int menuItem, void *callbackData,int panel)
{
	CmtGetLock(log_lock);
	if(dataLog[0]!='\0')
		if(!ConfirmPopup ("Warrning!", "There are already store log data if you start loging it will remove the old data are you sure you want to continue?"))
		{
			CmtReleaseLock(log_lock); 
			return;
		}
	CmtReleaseLock(log_lock);
	double *t=malloc(sizeof(double));
	char *endptr;
	char answer[BUFFER_SIZE];
	PromptPopup ("Loging", "Please enter interal number", answer, BUFFER_SIZE-1);
	
	answer[strcspn(answer, " ,\n")] = 0;
	*t=strtod(answer, &endptr);
	CmtGetLock(log_ch_lock);
	logChecker=1;
	CmtReleaseLock(log_ch_lock);
	CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE, Loging, (void *)t, &LogingThread);
	SetMenuBarAttribute (menuBar,menuItem , ATTR_CALLBACK_FUNCTION_POINTER, MyStopLog);
	SetMenuBarAttribute (menuBar,menuItem , ATTR_ITEM_NAME, "Stop loging");
}

void CVICALLBACK MyStopLog (int menuBar, int menuItem, void *callbackData,
							  int panel)
{
	CmtGetLock(log_lock);
	if(dataLog[0]!='\0')
		if(!ConfirmPopup ("Warrning!", "There are already store log data if you start loging it will remove the old data are you sure you want to continue?"))
		{
			CmtReleaseLock(log_lock); 
			return;
		}
	CmtReleaseLock(log_lock); 
	
	CmtGetLock(log_ch_lock);
	logChecker=0;
	CmtReleaseLock(log_ch_lock);
	CmtWaitForThreadPoolFunctionCompletion (DEFAULT_THREAD_POOL_HANDLE, LogingThread, OPT_TP_PROCESS_EVENTS_WHILE_WAITING);
	SetMenuBarAttribute (menuBar,menuItem , ATTR_CALLBACK_FUNCTION_POINTER, MyStartLog);
	SetMenuBarAttribute (menuBar,menuItem , ATTR_ITEM_NAME, "Start loging");
}

int CVICALLBACK MyTimer (int panel, int control, int event,
						 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_TIMER_TICK:

			AlertManager(RetVal);
			
			char RecBuffer[SEND_BUFFER_SIZE];
			unsigned char commade;
			char *massage;
			int l=ComRdTerm (mainInfo->portnumber, RecBuffer, SEND_BUFFER_SIZE,'\r')-1;
			commade=RecBuffer[0];
			massage=RecBuffer+1;
			switch (commade)
			{
				case List_of_Sensors_Response:
					 StopWatchDog();
					 if(strlen(massage)>0)
				 	 {
						UpdateSensorsSections(massage,l);
					 	if ((panelMonitor = LoadPanel (0, "Monitor_panel.uir", Monitor)) < 0)
							return -1;
					 	HidePanel (panelConnect);
					 	CreateSections(panelMonitor,&mainInfo->sectionHead,mainInfo); 
					 	DisplayPanel (panelMonitor);
					 }else MessagePopup ("Error", "Error a connection has been made but no sensor attach to the device");
					 
					break;
				case Sensor_Data_Response:
						UpdateSensorsData(massage,l);
						UpdateSectionData(panelMonitor,mainInfo,massage,NULL);
					break;
				case Data_Stream_Response:
						ResetWatchDog();
						UpdateSensorsData(massage,l);
						if(panelMonitor!=-1) UpdateAllSectionData(panelMonitor,mainInfo);
				break;
					
			}
			RecBuffer[0]=0;
			RecBuffer[1]=0;
			RecBuffer[2]='\r';
			
			break;
	}
	return 0;
}

void UpdateSensorsSections(char massage[],int l){
	int i=0;
	
	SensorType type;
	char id[MAX_SIZE_OF_ID];
	while(i<l)
	{
		int j=0;
		while(massage[i+j]!=0&&j<MAX_SIZE_OF_ID)
		{
			id[j]=massage[i+j];
			j++;
		}
		if(j==MAX_SIZE_OF_ID)return;
		i+=j;
		id[j]=massage[i++];
		type=(SensorType)massage[i++];
		CmtGetLock(sensor_lock);
		Node *sensorHead=addSensor(mainInfo->sensorHead,id,type);
		CmtReleaseLock(sensor_lock);
		if((mainInfo->sensorHead)==NULL)
			mainInfo->sensorHead=sensorHead;
			
	}
	
	return;	
}
void UpdateSensorsData(char massage[],int l){
		int i=0;
	
	SensorType type;
	char id[MAX_SIZE_OF_ID];
	while(i<l)
	{
		int j=0;
		while(massage[i+j]!=0)
		{
			id[j]=massage[i+j];
			j++;
		}
		i+=j;
		id[j]=massage[i++];
		double data=*((double *)(massage+i));
		i+=sizeof(double);
		CmtGetLock(sensor_lock);
		setSensorData(mainInfo->sensorHead,id,data);
		CmtReleaseLock(sensor_lock);
	}	
}
void StartWatchDog(Task task)
{
	Task *alert=malloc(sizeof(Task));
	*alert=task;
	CmtGetLock (wg_lock);
	threadChecker=1;
	CmtReleaseLock (wg_lock);
	CmtScheduleThreadPoolFunction (DEFAULT_THREAD_POOL_HANDLE, WatchDog, ((void*)alert), &WatchDogThread);
}
void StopWatchDog()
{
	CmtGetLock (wg_lock);
	threadChecker=-1;
	CmtReleaseLock (wg_lock);
	
	
}
void ResetWatchDog()
{
	CmtGetLock (wg_lock);
	threadChecker=-2;
	CmtReleaseLock (wg_lock);

}

static int WatchDog(void *task)
{
	int maxcount =10;
	Task tmpA=*(Task*)task;
	Task alert=tmpA;
	free(task);
	double t=(5000*1e-3)/maxcount;
	CmtGetLock (wg_lock);
	watchDogCounter=0;
	while(threadChecker==1)
	{
		CmtReleaseLock (wg_lock); 
		watchDogCounter++;
		Delay(t);
		CmtGetLock (wg_lock);
		if(watchDogCounter>=maxcount&&threadChecker==1) 
			threadChecker=0;
		if(threadChecker==-2)
		{
			watchDogCounter=0;
			threadChecker=1;
		}
	}
	
	
	if(threadChecker!=0) 
		alert=0; 
	RetVal=alert;
	CmtReleaseLock (wg_lock);
	CmtExitThreadPoolThread (0);
	return 0;
	
}

void AlertManager(Task task)
{
	int alertMassage;
	char msg[20];
	switch (task)
	{
		case waitForList:
			
			MessagePopup ("Alert", "Time out\nNo respond from device");
			StopWatchDog();
			if(panelMonitor!=-1) 
			{
				DiscardPanel(panelMonitor);
				panelMonitor=-1;
			}
			DisplayPanel(panelConnect);
			break;
		case waitForMassage:
			int select = GenericMessagePopup ("Alert", "Time out\nNo respond from device!\nWould you like to:", "Exit Program", "Disconnect", "keep waiting", NULL,0 , 0, VAL_GENERIC_POPUP_BTN1, VAL_GENERIC_POPUP_BTN1, VAL_GENERIC_POPUP_BTN3);
			
			switch (select)
			{
				case VAL_GENERIC_POPUP_BTN1:
					QuitUserInterface (0);
					break;
				case VAL_GENERIC_POPUP_BTN2:
					msg[0]=Stop_Data_Stream; 
					msg[1]='\r';
					ComWrt (mainInfo->portnumber, msg, 2);	
					freeCtrlSection(panelMonitor,mainInfo->sectionHead);
					CmtGetLock(sensor_lock); 
					freelist(mainInfo->sensorHead);
					CmtReleaseLock(sensor_lock); 
					DiscardPanel(panelMonitor);
					panelMonitor=-1;
					DisplayPanel(panelConnect);
					break;
				case VAL_GENERIC_POPUP_BTN3:
					StartWatchDog(waitForMassage);
					msg[0]=Start_Data_Stream;
					memcpy(msg+1, &t_t, sizeof(double)); 
					msg[sizeof(double)+1]='\r';
					ComWrt (mainInfo->portnumber, msg, sizeof(double)+2);
					break;
			}
			break;
		
	}
	RetVal=0;

}


void GetCurrentTimeAsString(char *timeStr) {
    time_t currentTime;
    struct tm *localTime;
    time(&currentTime);

    localTime = localtime(&currentTime);

    strftime(timeStr, 50, "%c", localTime);
}

static int Loging(void *intervals)
{
	double t=*(double*)intervals;
	char tmpBuff[MAX_LOG_LENGHT];
	CmtGetLock(log_lock);
	sprintf(dataLog,"Time");
	
	CmtGetLock(sensor_lock);
	Node *tmp=mainInfo->sensorHead;  
	while(tmp!=NULL)
	{
		char* type=getSensorTypeString(getSensorType(mainInfo->sensorHead,tmp->sensor->id));
		sprintf(tmpBuff,"%s,%s Type: %s",dataLog,tmp->sensor->id,type);
		strcpy(dataLog,tmpBuff);
		tmp=tmp->next;
	}
	sprintf(tmpBuff,"%s\n",dataLog);
	strcpy(dataLog,tmpBuff);
	
	CmtReleaseLock(sensor_lock);
	CmtReleaseLock(log_lock);
	
	
	CmtGetLock(log_ch_lock);
	while(logChecker)
	{
		CmtReleaseLock(log_ch_lock);
		CmtGetLock(sensor_lock);
		CmtGetLock(log_lock); 
		tmp=mainInfo->sensorHead;
		char timestr[BUFFER_SIZE];
		GetCurrentTimeAsString(timestr);
		sprintf(tmpBuff,"%s%s",dataLog,timestr);
		strcpy(dataLog,tmpBuff);
		while(tmp!=NULL)
		{
			sprintf(tmpBuff,"%s,%.3f",dataLog,tmp->sensor->data);
			strcpy(dataLog,tmpBuff);
			tmp=tmp->next;
		}
		CmtReleaseLock(sensor_lock);
		sprintf(tmpBuff,"%s\n",dataLog);
		strcpy(dataLog,tmpBuff);
		CmtReleaseLock(log_lock);
		
		Delay(t);
		CmtGetLock(log_ch_lock); 
	}
	CmtReleaseLock(log_ch_lock);
	free(intervals);
	CmtExitThreadPoolThread (0);
	
	 
}
