
extern uint8_t ADDR_BYTES;
extern char *ErrorCode;
extern unsigned int ORIGIN;

extern void markUndefLabel(const uint8_t *data);
extern uint8_t *checkIncludes(const char *name);

uint8_t *OpcodesA(const char *line);
uint8_t *OpcodesB(const char *line);
uint8_t *OpcodesC(const char *line);
uint8_t *OpcodesD(const char *line);
uint8_t *OpcodesE(const char *line);
uint8_t *OpcodesF(const char *line);
uint8_t *OpcodesI(const char *line);
uint8_t *OpcodesJ(const char *line);
uint8_t *OpcodesL(const char *line);
uint8_t *OpcodesM(const char *line);
uint8_t *OpcodesN(const char *line);
uint8_t *OpcodesO(const char *line);
uint8_t *OpcodesP(const char *line);
uint8_t *OpcodesR(const char *line);
uint8_t *OpcodesS(const char *line);
uint8_t *OpcodesT(const char *line);
uint8_t *OpcodesX(const char *line);
uint8_t *OpcodesNone(const char *line);

int getNumber(char **line);
int isNumber(const char *line);
int isIrOff(const char *line);
uint8_t getIrOff(const char **line);
uint8_t getCondition(const char **line);
int digitValue(char c);
uint8_t checkRRArg(const char *args,uint8_t base);
uint8_t checkRArg(const char *args,uint8_t base);
uint8_t getRArgN(const char *args);

void clearBuffer(void);
uint8_t *invalidArgument(void);
