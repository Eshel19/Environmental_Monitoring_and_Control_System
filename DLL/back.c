#include <cvidef.h>
#include <ansi_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#define MAX_SIZE_OF_ID 10


DLLEXPORT int NUMBER_OF_SENSORS_TYPES = 6;

DLLEXPORT typedef enum {
    TEMPERATURE_SENSOR,
    HUMIDITY_SENSOR,
    PRESSURE_SENSOR,
    GAS_SENSOR,
    LIGHT_SENSOR,
    SOUND_SENSOR
} SensorType;

DLLEXPORT typedef struct {
    char id[MAX_SIZE_OF_ID];
    SensorType type;
    double data;
} Sensor;


DLLEXPORT typedef struct Node {
    Sensor *sensor;
    struct Node *next;
} Node;

Sensor* initializeNewSensor(char *id, SensorType type) {
	Sensor* newSensor=malloc(sizeof(Sensor));
	if (newSensor == NULL) {
        return NULL;
    }
    strncpy(newSensor->id, id, MAX_SIZE_OF_ID*sizeof(char));
    newSensor->type = type;
    newSensor->data = 0.0;
	return newSensor;
}
Node* addSensor(Node *head, char *id, SensorType type) {
	Node *newNode = (Node*) malloc(sizeof(Node));
	if(newNode==NULL) return NULL; //Cannot allocate memory for new newNode
	newNode->sensor=initializeNewSensor(id, type);
	if(newNode->sensor==NULL) return NULL; //Cannot allocate memory for new sensor 
	newNode->next=NULL;
	if(head==NULL) head=newNode; 	
	else
	{
		while(head->next!=NULL) head=head->next; 
		head->next=newNode;
		
	}
    return newNode;
}

void freelist(Node *head)
{
	if(head!=NULL)
	{
		freelist(head->next);
		free(head->sensor);
		free(head);
	}
}

Node* removeSensor(Node *head, char *id) {
    Node *current = head;
    Node *previous = NULL;

    while (current != NULL && strcmp(current->sensor->id, id) != 0) {
        previous = current;
        current = current->next;
    }

    if (current == NULL) {
        return head;
    }

    if (previous == NULL) {
        head = current->next;
    } else {
        previous->next = current->next;
    }

    free(current);
    return head;
}

Sensor* findSensor(Node *head, char *id) {
    Node *current = head;
    while (current != NULL) {
        if (strcmp(current->sensor->id, id) == 0) {
            return current->sensor;
        }
        current = current->next;
    }
    return NULL;
}

double getSensorData(Node *head, char *id) {
    Sensor *sensor = findSensor(head, id);
    if (sensor != NULL) {
        return sensor->data;
    }
    return DBL_MIN; // Indicate that the sensor was not found return minimum value of a double
}

int setSensorData(Node *head, char *id,double data) {
    Sensor *sensor = findSensor(head, id);
    if (sensor != NULL) {
		sensor->data=data;
        return 0;
    }
    return -1; // Indicate that the sensor was not found return -1
}

SensorType getSensorType(Node *head, char *id) {
    Sensor *sensor = findSensor(head, id);
    if (sensor != NULL) {
        return sensor->data;
    }
    return sensor->type; // Indicate that the sensor was not found return minimum value of a double
}

char* getSensorTypeString(SensorType type) {
    switch (type) {
        case TEMPERATURE_SENSOR:
            return "Temperature";
        case HUMIDITY_SENSOR:
            return "Humidity";
        case PRESSURE_SENSOR:
            return "Pressure";
        case GAS_SENSOR:
            return "Gas";
        case LIGHT_SENSOR:
            return "Light";
        case SOUND_SENSOR:
            return "Sound";
    }
	return "Unknown"; 
}


