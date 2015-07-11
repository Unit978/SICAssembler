
                /* Define a few constants */
#define TRUE    1                       /* Boolean constants */
#define FALSE   0
#define MSIZE   32768L

                /* Define some useful data types */
typedef unsigned char   BYTE;
typedef BYTE            WORD[3];
typedef BYTE            FLOAT[4];
typedef unsigned char   BOOLEAN;
typedef unsigned long   ADDRESS;

extern void GetMem (ADDRESS, BYTE*, int);
extern void PutMem (ADDRESS, BYTE*, int);
extern void GetReg (WORD*);
extern void PutReg (WORD*);
extern ADDRESS GetPC (void);
extern void GetIR (ADDRESS, char *);
extern char GetCC (void);
extern void PutPC (ADDRESS);
extern void SICInit (void);
extern void SICRun (ADDRESS *, BOOLEAN);
