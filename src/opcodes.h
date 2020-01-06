
#define SIZEOF_DEFINE_T 18
#define F_ARG_BYTE 7
#define F_OFFSET_ARG 1<<3
#define F_LONG_ARG 1<<4
#define F_BYTE_ARG 1<<5
#define F_DIRECT_CMP 1<<7
typedef struct _define_entry_{
	uint8_t bytes;
	uint8_t emit[4];
	uint8_t flags;
	char opcode[12];
}define_entry_t;


extern const char *MemoryError;

extern uint8_t ADDR_BYTES;
extern char *ErrorCode;
extern char *LAST_LINE;
extern unsigned int ORIGIN;
extern uint8_t CURRENT_BYTES;
extern ti_var_t gfp;

extern define_entry_t *internal_define_pointers[26];

extern uint8_t *checkIncludes(const char *name);
extern void setGotoOffset(const char *name);
extern void defineGoto(uint8_t *name,int val);
extern int includeFile(const char *fname);

char *processOpcodeLine(const char *name);
int getArgFromLine(const char *line);
uint8_t *checkInternal(const char *line,define_entry_t **endptr);
void emitArgument(uint8_t *buf,const char *line,uint8_t flags);
bool isRegister(const char *name);

int getNumberWrapper(char **line);
int getNumber(char **line);
uint8_t getCondition(const char **line);
int digitValue(char c);
uint8_t checkRRArg(const char *args,uint8_t base);
uint8_t checkRArg(const char *args,uint8_t base);
uint8_t getRArgN(const char *args);

char *getWord(const char **line);
