#include <cvidef.h>
#include <ansi_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <userint.h>
#include "sensors.h"  


#define DISTANT_BETWEEN_SECTION 40
#define DISTANT_BETWEEN_CTRL 90
#define DISTANT_FROM_LEFT 20
#define DISTANT_FROM_TOP 30

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


Section* initializeNewSection(int panel,char *id,int inx); 
void RemoveSectionNode(int panel,SectionNode **sectionHead,Node **sensorhead, int index);
SectionNode* addSection(int panel,SectionNode *sectionHead, char *id,int inx);
int FindSectionIndexByUpdateButton(SectionNode *head, int updateButton_ctrl);
int FindSectionIndexByRemoveButton(SectionNode *head, int removeButton_ctrl);
int UpdateSectionByUpdateButton(int panel, int updateButton_ctrl,SectionNode *sectionhead,Node *sensorHead);



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
	removeSensor(sensorhead,current->section->id_sensor);
	
	
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
		Node *tmpNode=sectionHead;
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

int UpdateSectionByUpdateButton(int panel, int updateButton_ctrl,SectionNode *sectionhead,Node *sensorHead) {
    SectionNode *current = sectionhead;
    while ((current != NULL)&&current->section->updateButton_ctrl != updateButton_ctrl) {
        current = current->next;
    }
	if(current==NULL) return -1; // Indicate that the section was not found
	char tmpId[MAX_SIZE_OF_ID];
	strcpy(tmpId,current->section->id_sensor);
	
	double a;
	GetCtrlVal (panel,current->section->data_ctrl , &a);
	setSensorData(sensorHead,tmpId,a);
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




void freeSections(SectionNode *head)
{
	if(head!=NULL)
	{
		freelist(head->next);
		free(head->section);
		free(head);
	}
}
