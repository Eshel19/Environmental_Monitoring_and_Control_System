#include <cvidef.h>
#define MAX_SIZE_OF_ID 10 

 
/************** Static Function Declarations **************/

/************** Global Variable Declarations **************/
extern DLLIMPORT int NUMBER_OF_SENSORS_TYPES; 


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

/************** Global Function Declarations **************/
extern Sensor *initializeNewSensor(char *id, SensorType type);
extern Node *addSensor(Node *head, char *id, SensorType type);
extern void freelist(Node *head);
extern void removeSensor(Node **head, char *id);
extern Sensor *findSensor(Node *head, char *id);
extern double getSensorData(Node *head, char *id);
extern int setSensorData(Node *head, char *id, double data);
extern char *getSensorTypeString(SensorType type);
extern SensorType getSensorType(Node *head, char *id);
extern char *getSensorTypeUnit(SensorType type);
