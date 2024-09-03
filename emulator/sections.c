#include <ansi_c.h>
#include <userint.h>
#include "sensors.h" 
#define MAX_SECTIONS 12
#define DISTANT_BETWEEN_SECTION 30
#define DISTANT_BETWEEN_CTRL 90
#define DISTANT_FROM_LEFT 10
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

Section* initializeNewSection(char *id,int inx,int panel) {
	Section* newSection=malloc(sizeof(Section));
	if (newSection == NULL) {
        return NULL;
    }
    strncpy(newSection->id_sensor, id, MAX_SIZE_OF_ID*sizeof(char));
    newSection->inx = inx;
    // Create controls dynamically
    newSection->id_ctrl = NewCtrl(panel, CTRL_STRING, "ID", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT);
    newSection->type_ctrl = NewCtrl(panel, CTRL_NUMERIC, "type", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT+DISTANT_BETWEEN_CTRL);
	newSection->data_ctrl = NewCtrl(panel, CTRL_NUMERIC, "data", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT+DISTANT_BETWEEN_CTRL*2);
    newSection->updateButton_ctrl = NewCtrl(panel, CTRL_SQUARE_COMMAND_BUTTON, "Update Value", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT+DISTANT_BETWEEN_CTRL*3);
    newSection->removeButton_ctrl = NewCtrl(panel, CTRL_SQUARE_COMMAND_BUTTON, "Remove", inx * DISTANT_BETWEEN_SECTION+DISTANT_FROM_TOP, DISTANT_FROM_LEFT+DISTANT_BETWEEN_CTRL*4);
    
	//set control mode and other setting for each ctrl
	GetCtrlAttribute (panel,newSection->id_ctrl , ATTR_CTRL_MODE, 0);
	GetCtrlAttribute (panel,newSection->type_ctrl , ATTR_CTRL_MODE, 0);
	
	
	
	return newSection;
}

void RemoveSectionNode(SectionNode **head, int index) {
    SectionNode *current = *head;
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
        *head = current->next;
    } else {
        // Otherwise, update the previous node's next pointer
        previous->next = current->next;
    }

    // Free the memory allocated for the node
    free(current->section);
    free(current);

    // Update the inx field of the remaining sections
    current = *head;
    int newIndex = 0;
    while (current != NULL) {
        current->section->inx = newIndex;
        newIndex++;
        current = current->next;
    }
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
