
#define SIZEOF_DEFINE_T 18
#define M_ARG_BYTE 7
#define F_OFFSET_ARG 1<<3
#define F_LONG_ARG 1<<4
#define F_BYTE_ARG 1<<5
#define F_JR_ARG 1<<6
#define F_DIRECT_CMP 1<<7
#define N_OPCODE_NAME_BYTES 12
typedef struct _define_entry_{
	uint8_t bytes;
	uint8_t emit[4];
	uint8_t flags;
	char opcode[12];
}define_entry_t;

#define SIZEOF_LABEL_T 13
#define M_NUM_BYTES 7
#define F_OFFSET_VALUE 1<<3
#define F_STRING_VALUE 1<<5
#define F_UNDEFD_VALUE 1<<6
#define F_EXPRESSION_VALUE 1<<7
typedef struct _label_{
	char *name;
	int value,org,offset;
	uint8_t bytes;
}label_t;

extern const char *MemoryError;
extern const char *UndefinedLabelError;
extern const char *NumberFormatError;
extern const char *NamespaceError;

extern uint8_t ADDR_BYTES;
extern char *ErrorCode;
extern char *ErrorWord;
extern char *LAST_LINE;
extern unsigned int ORIGIN;
extern unsigned int O_FILE_TELL;
extern uint8_t CURRENT_BYTES;
extern ti_var_t gfp;
extern char *NAMESPACE;
	

extern define_entry_t *internal_define_pointers[26];

extern uint8_t *checkIncludes(const char *name);
extern void defineGoto(void *val,int offset);
extern label_t *findLabel(const char *name);
extern int getLabelValue(label_t *lbl);

extern void message(const char *msg);
extern void stack_push(int val);
extern int stack_pop(void);

char *processOpcodeLine(const char *name);
char *getArgFromLine(const char *line);
char *processDataLine(const char *line,char cc);
uint8_t *checkNoArgOpcode(const char *line,define_entry_t **endptr);
uint8_t *checkOpcodeMatches(define_entry_t *def,char *name);
bool isRegister(const char *name);
bool isCondition(const char *name);

int getNumber(char **line,label_t *gt,bool jr);
int getNumberNoMath(char **line,uint8_t *base);
int digitValue(char c);
char *getWord(const char **line);
char *skipWord(char *line);
