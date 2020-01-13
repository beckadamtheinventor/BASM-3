
#define SIZEOF_DEFINE_T 18
#define F_ARG_BYTE 7
#define F_OFFSET_ARG 1<<3
#define F_LONG_ARG 1<<4
#define F_BYTE_ARG 1<<5
#define F_JR_ARG 1<<6
#define F_DIRECT_CMP 1<<7
typedef struct _define_entry_{
	uint8_t bytes;
	uint8_t emit[4];
	uint8_t flags;
	char opcode[12];
}define_entry_t;

#define SIZEOF_LABEL_T 13
typedef struct _label_{
	char *name;
	int value,offset,org;
	uint8_t bytes;
}label_t;


extern const char *MemoryError;
extern const char *UndefinedLabelError;
extern const char *NumberFormatError;

extern uint8_t ADDR_BYTES;
extern char *ErrorCode;
extern char *ErrorWord;
extern char *LAST_LINE;
extern unsigned int ORIGIN;
extern unsigned int O_FILE_TELL;
extern uint8_t CURRENT_BYTES;
extern ti_var_t gfp;
extern char *NAMESPACE;
extern unsigned int NAMESPACE_LEN;
	

extern define_entry_t *internal_define_pointers[27];

extern uint8_t *checkIncludes(const char *name);
extern void defineGoto(void *val,int offset);
extern label_t *findLabel(const char *name);
extern int getLabelValue(label_t *lbl);

char *processOpcodeLine(const char *name);
char *getArgFromLine(const char *line);
uint8_t *checkInternal(const char *line,define_entry_t **endptr);
bool isRegister(const char *name);
bool isCondition(const char *name);

int getNumber(char **line,label_t *gt,bool jr);
int getNumberNoMath(char **line,uint8_t *base);
int digitValue(char c);
char *getWord(const char **line);
