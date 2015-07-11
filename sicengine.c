
/*
 SIC simulator, version 1.5
 revised 10/27/89
 translated to C  Summer 1994  (David Egle - Univ Texas Pan American)

 This version of the SIC simulator includes all SIC/XE instructions
 and capabilities except for the following:

    1. floating-point data type and instructions (ADDF, COMPF, DIVF, FIX,
       FLOAT, LDF, MULF, NORM, STF, SUBF)
    2. i/o channels and associated instructions (SIO, TIO, HIO)
    3. interrupts and associated instructions (LPS, STI, SVC)
    4. certain features associated with register SW (user/supervisor modes,
       running/idle states)
    5. virtual memory and associated instructions (LPM)
    6. memory protection and associated instructions (SSK)

 For a simulator that supports only standard SIC features, set the
 global constant XE to FALSE.

 The simulator uses the following external files:

    Log     -- log of commands entered and results displayed (may be printed
               to obtain hard copy record of terminal session)
    DevBoot -- represents device 00 (bootstrap object code)
    Dev[6]  -- represent the input/output devices
               indices 0, 1, and 2 are the input devices, corresponding
                   to the files devf1, devf2, and devf3
               while 3, 4, and 5 are the output devices, corresponding
                   to the files dev04, dev05, dev06
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

                /* Define a few constants */
#define TRUE    1                       /* Boolean constants */
#define FALSE   0
#define MSIZE   32768L                  /* maximum memory size */
                                        /* allows use of short for addresses */
#define LT      1                       /* condition codes */
#define EQ      2                       /*  used in status word */
#define GT      3
#define XE      TRUE                    /* determines if XE features */
                                        /*  are supported */

                /* Define some useful data types */
typedef unsigned char   BYTE;
typedef BYTE            WORD[3];
typedef BYTE            FLOAT[4];
typedef unsigned char   BOOLEAN;
typedef unsigned long   ADDRESS;
typedef char*           MESSAGE;
typedef struct {
        char OP[7];
        short FORM;
     } OPCODE;

                /* The opcode table */
OPCODE Ops[] = {{"LDA   ", 3}, {"LDX   ", 3}, {"LDL   ", 3}, {"STA   ", 3},
                {"STX   ", 3}, {"STL   ", 3}, {"ADD   ", 3}, {"SUB   ", 3},
                {"MUL   ", 3}, {"DIV   ", 3}, {"COMP  ", 3}, {"TIX   ", 3},
                {"JEQ   ", 3}, {"JGT   ", 3}, {"JLT   ", 3}, {"J     ", 3},
                {"AND   ", 3}, {"OR    ", 3}, {"JSUB  ", 3}, {"SUB   ", 3},
                {"LDCH  ", 3}, {"STCH  ", 3}, {"ADDF  ", 3}, {"SUBF  ", 3},
                {"MULF  ", 3}, {"DIVF  ", 3}, {"LDB   ", 3}, {"LDS   ", 3},
                {"LDF   ", 3}, {"LDT   ", 3}, {"STB   ", 3}, {"STS   ", 3},
                {"STF   ", 3}, {"STT   ", 3}, {"COMPF ", 3}, {"      ", 0},
                {"ADDR  ", 2}, {"SUBR  ", 2}, {"MULR  ", 2}, {"DIVR  ", 2},
                {"COMPR ", 2}, {"SHIFTL", 2}, {"SHIFTR", 2}, {"RMO   ", 2},
                {"SVC   ", 2}, {"CLEAR ", 2}, {"TIXR  ", 2}, {"      ", 0},
                {"FLOAT ", 1}, {"FIX   ", 1}, {"NORM  ", 1}, {"      ", 0},
                {"LPS   ", 3}, {"STI   ", 3}, {"RD    ", 3}, {"WD    ", 3},
                {"TD    ", 3}, {"      ", 0}, {"STSW  ", 3}, {"SSK   ", 3},
                {"SIO   ", 1}, {"HIO   ", 1}, {"TIO   ", 1}, {"      ", 0}};

                /* Define the CPU variables */
WORD Registers[6];      /* holds registers A, X, L, B, S, T */
ADDRESS PC;             /* holds PC */
WORD Status;            /* status word */
FLOAT Fl;               /* Floating point accumulator */

                /* Input/Output variables */
FILE  *DevBoot;
FILE *Dev[6];
char *SICFile[6] = {"devf1", "devf2", "devf3", "dev04", "dev05", "dev06"};
BYTE Wait[6];
BOOLEAN Init[6],
        EndFile[6];
BYTE InTab[256], OutTab[256];

                /* Memory variables */
BYTE Memory[MSIZE];     /* main memory */
ADDRESS MAR;            /* memory address register */
WORD MBR;               /* memory buffer register */

                /* Miscellaneous variables */
WORD Word1;             /* holds the constant 1 */
BOOLEAN ERROR;          /* generic error flag */
char *Msg[16];             /* holds error messages */

                /* Function prototypes */
                                            /* first the user interface */
void GetMem (ADDRESS, BYTE*, int);
void PutMem (ADDRESS, BYTE*, int);
void GetReg (WORD*);
void PutReg (WORD*);
ADDRESS GetPC (void);
void PutPC (ADDRESS);
void GetIR (ADDRESS, char *);
char GetCC (void);
void SICRun (ADDRESS *, BOOLEAN);
void SICInit (void);
                                            /* now the internal routines */
void SICError (int);
int SICEoln (FILE *);
void GetAddr(int, WORD, BOOLEAN, ADDRESS *);
void GetData(int, WORD, BOOLEAN, BOOLEAN, WORD, ADDRESS *);
void Shift (BYTE *, int, int);
void Negl (BYTE *);
void Addl (BYTE *, BYTE *, BYTE *);
void Subl (BYTE *, BYTE *, BYTE *);
void Mull (BYTE *, BYTE *, BYTE *);
void Divl (BYTE *, BYTE *, BYTE *);
void Compl (BYTE *, BYTE *);
void Load (int, WORD, BOOLEAN, BOOLEAN, int *, WORD, ADDRESS *);
void Store (int, WORD, BOOLEAN, BOOLEAN, int *, ADDRESS *);
void Jump (int, WORD, BOOLEAN, BOOLEAN, ADDRESS *);
void Arith (int, WORD, BOOLEAN, BOOLEAN, WORD, ADDRESS *);
void Logic (int, WORD, BOOLEAN, BOOLEAN, WORD, ADDRESS *);
void CharIO (int, WORD, BOOLEAN, BOOLEAN, WORD, ADDRESS *);
void RegReg (int, int, int);
void RegMan (int, int, int);
void SICExec (int, int, int, WORD, BOOLEAN, BOOLEAN);
void SICStart (void);
void DecMode (BOOLEAN *, BOOLEAN *, BOOLEAN *, BOOLEAN *, BOOLEAN *,
        BOOLEAN *, BOOLEAN *, BOOLEAN, BOOLEAN);
void DecAddr (WORD, BOOLEAN *, BOOLEAN *, BOOLEAN *, BOOLEAN *, BOOLEAN *,
        BOOLEAN, BOOLEAN, ADDRESS);
void SICFetch (int *, int *, int *, WORD, BOOLEAN *, BOOLEAN *, BOOLEAN *,
        BOOLEAN *, BOOLEAN *, BOOLEAN *);

/******************************************************************/
void SICError (int n)
{
  /* Sets the appropriate error bits in the status word
     and displays an appropriate error message */

     Status[2] = (Status[2] & 0xF) | n;
     printf("\n\nAt PC = %x: %s\n\n", PC, Msg[n]);
     ERROR = TRUE;
}

/******************************************************************/

void GetMem (ADDRESS Addr, BYTE *Data, int Mode)
{
  int i;

     if (Addr < 0 || Addr > MSIZE) {
         SICError(3);
         return;
     }
     if (Mode == 0)
         Data[0] = Memory[Addr];
     else
         for (i = 0; i <= 2; i++)
             Data[i] = Memory[Addr + i];
}

/******************************************************************/

void PutMem (ADDRESS Addr, BYTE *Data, int Mode)
{
  int i;

     if (Addr < 0 || Addr > MSIZE) {
         SICError(3);
         return;
     }
     if (Mode == 0)
         Memory[Addr] = Data[0];
     else
         for (i = 0; i <= 2; i++)
             Memory[Addr + i] = Data[i];
}

/******************************************************************/

void GetReg (WORD *Regs)
{
  int i, j;

     for (i = 0; i < 6; i++)
         for (j = 0; j < 3; j++)
             Regs[i][j] = Registers[i][j];
}

/******************************************************************/

void PutReg (WORD *Regs)
{
  int i, j;

     for (i = 0; i < 6; i++)
         for (j = 0; j < 3; j++)
             Registers[i][j] = Regs[i][j];
}

/******************************************************************/

ADDRESS GetPC (void)
{
     return PC;
}

/******************************************************************/

void PutPC (ADDRESS Addr)
{
     if (Addr < 0 || Addr > MSIZE) {
         SICError(3);
         return;
     }
     PC = Addr;
}

/******************************************************************/

void GetIR (ADDRESS Addr, char * Inst)
{
  int OpCode, R1, R2;
  long Oper;
  short N, I, X, B, P, E;
  WORD IR;
  short Temp;
  char REG[][2] = {"A", "X", "L", "B", "S", "T"};
  char Build[12];

    GetMem (Addr, IR, 1);
    Temp = IR[0];
    OpCode = Temp >> 2 & 0x3f;
    N = Temp >> 1 & 1;
    I = Temp & 1;
    Temp = IR[1];
    X = Temp >> 7 & 1;
    B = Temp >> 6 & 1;
    P = Temp >> 5 & 1;
    E = Temp >> 4 & 1;
    switch (Ops[OpCode].FORM) {
        case 1:                 /* No operands */
                sprintf(Inst, "%6s", Ops[OpCode].OP);
                break;
        case 2:                 /* Register-Register */
        R1 = IR[1] >> 4 & 0x7;
        R2 = IR[1] & 0x7;
                sprintf(Inst, "%6s  %c,%c", Ops[OpCode].OP, REG[R1], REG[R2]);
                break;
        case 3:                 /* Format 3 & 4 and SIC */
                if (N == 0 && I == 0) {         /* SIC */
            Oper = (IR[1] & 0x7f) * 256 + IR[2];
            sprintf(Inst, "%6s  [%04lx]%s", Ops[OpCode].OP, Oper,
                                (X == 1) ? ",X" : " ");
                    break;
                }
                Build[0] = '\0';
                if (!(N == 1 && I == 1) )
                    if (N == 1)
                        strcat(Build, "@");
                    else
                        strcat(Build, "#");
                if (B == 1)
                    strcat(Build, "(B)");
                if (P == 1)
                    strcat(Build, "(PC)");
                Oper = (IR[1] & 0xf) * 256 + IR[2];
                if (E == 1) {                   /* extended form */
                    GetMem (Addr + 3, IR, 0);
                    Oper = Oper * 256 + (IR[0] & 0xff);
            sprintf(Inst, "+%6s  %s%05lx", Ops[OpCode].OP, Build,
                                        Oper);
                } else
            sprintf(Inst, "%6s  %s%03lx", Ops[OpCode].OP, Build,
                                        Oper);
                if (X == 1)
                    strcat(Inst, ",X");
    }
}

/******************************************************************/

char GetCC(void)
{
    switch (Status[2] >> 6 & 0x3) {
        case 1: return ('<');
        case 2: return ('=');
        case 3: return ('>');
        default: return ('?');
    }
}

/******************************************************************/

int SICEoln(FILE *f)
{
  /* Check if at the end of line (or end of entire file). */
  int ch;

     ch = fgetc(f);
     if (ch == EOF)
         return 1;
     ungetc(ch, f);
     return (ch == '\n');
}

/******************************************************************/

void GetAddr(int opcode, WORD targaddr, BOOLEAN indir, ADDRESS *opaddr)
{
  /* This procedure gets the main memory address to be used for
     instruction execution, including indirection if applicable,
     placing it in 'opaddr'*/

  int i;
  ADDRESS temp;

     *opaddr = 0;
     i = 0;
     while (!ERROR && i < 3)
         if (MSIZE < 256 * (*opaddr) + targaddr[i]) {
             SICError(3);  /* address out of range */
         } else {
             *opaddr = 256 * (*opaddr) + targaddr[i];
             i++;
         }
     if (indir && !ERROR) {
         temp = 0;
         i = 0;
         while (!ERROR && i <= 2)
             if (MSIZE < 256 * temp + Memory[*opaddr + i]) {
                 SICError(3);  /* address out of range */
             } else {
                 temp = 256 * temp + Memory[*opaddr + i];
                 i++;
             }
         if (!ERROR)
             *opaddr = temp;
     }
     if (*opaddr > MSIZE - 2 && !((opcode == 80 /*ldch*/)
              || (opcode == 84 /*stch*/) || (opcode == 216 /*rd*/)
              || (opcode == 220 /*wd*/) || (opcode == 224 /*td*/))) {
         SICError(3);  /* address out of range */
     }
} /*GetAddr*/

/******************************************************************/

void GetData(int opcode, WORD targaddr, BOOLEAN indir, BOOLEAN immed,
 WORD data, ADDRESS *opaddr)
{
  /* This procedure fetches an operand from memory address
     opaddr, placing it in 'data'. if the instruction specified
     immediate addressing, the operand value is obtained from
     the instruction (targaddr) instead of from memory. */

  int i;

     if (immed)
         for (i = 0; i < 3; i++)
             data[i] = targaddr[i];
     else {
         GetAddr(opcode, targaddr, indir, opaddr);
         if (ERROR)
             for (i = 0; i < 3; i++)
                 data[i] = 0;
         else
             for (i = 0; i < 3; i++)
                 data[i] = Memory[*opaddr + i];
     }
} /*GetData*/

/******************************************************************/

void Shift (BYTE *op, int n, int stype)
{
  /* This procedure shifts op left or right n bit positions.
     If stype = 0, the shift is left circular; if stype = 1, the
     shift is right with sign extension. */

  int carry, temp, i, j;

     if (stype == 0)
         for (i = 1; i <= n; i++) {
             temp = 2 * op[2];
             op[2] = temp & 255;
             carry = temp / 256;
             temp = 2 * op[1] + carry;
             op[1] = temp & 255;
             carry = temp / 256;
             temp = 2 * op[0] + carry;
             op[0] = temp & 255;
             carry = temp / 256;
             op[2] += carry;
         }
     if (stype == 1)
         for (i = 1; i <= n; i++) {
             temp = op[0];
             op[0] = temp / 2;
             carry = temp & 1;
             if (temp > 127)
                 op[0] += 128;
             temp = op[1];
             op[1] = temp / 2 + 128 * carry;
             carry = temp & 1;
             temp = op[2];
             op[2] = temp / 2 + 128 * carry;
             carry = temp & 1;
         }
} /*Shift*/

/******************************************************************/

/*  The following procedures -- Negl, Addl, Subl, Mull, Divl, Compl --
    perform integer arithmetic operations on operands of type word. They
    are included so that this simulator can be run on machines that
    cannot directly represent 24-bit integers. */

void Negl (BYTE *op)                                            /*negate*/
{
   int i;
   WORD res;

     for (i = 0; i < 3; i++)
         res[i] = 255 - op[i];
     if (res[2] == 255) {
         res[2] = 0;
         if (res[1] == 255) {
             res[1] = 0;
             if (res[0] == 255)
                 res[0] = 0;
             else
                 res[0]++;
         } else
             res[1]++;
     } else
         res[2]++;
     for (i = 0; i < 3; i++)
         op[i] = res[i];
} /*Negl*/

/******************************************************************/

void Addl(BYTE *op1, BYTE *op2, BYTE *result)                      /*add*/
{
  int i, temp, carry;
  WORD res;

     temp = op1[2] + op2[2];
     if (temp <= 255) {
         res[2] = temp;
         carry = 0;
     } else {
         res[2] = temp - 256;
         carry = 1;
     }
     temp = op1[1] + op2[1] + carry;
     if (temp <= 255) {
         res[1] = temp;
         carry = 0;
     } else {
         res[1] = temp - 256;
         carry = 1;
     }
     temp = op1[0] + op2[0] + carry;
     if (temp <= 255)
         res[0] = temp;
     else
         res[0] = temp - 256;
     if (((op1[0] >= 128) && (op2[0] >= 128) && (res[0] < 128))
              || ((op1[0] < 128) && (op2[0] < 128) && (res[0] >= 128))) {
         SICError(2);   /* arithmetic overflow */
     } else
         for (i = 0; i < 3; i++)
             result[i] = res[i];
} /*Addl*/

/******************************************************************/

void Subl (BYTE *op1, BYTE *op2, BYTE *result)                 /*subtract*/
{
  WORD res, temp2;
  int i;

     for (i = 0; i < 3; i++)
         temp2[i] = op2[i];
     Negl(temp2);
     Addl(op1, temp2, res);
     for (i = 0; i < 3; i++)
         result[i] = res[i];
} /*Subl*/

/******************************************************************/

void Mull (BYTE *op1, BYTE *op2, BYTE *result)                 /*multiply*/
{
  int i;
  WORD temp1, temp2, res;

     for (i = 0; i < 3; i++)
         res[i] = 0;
     for (i = 0; i < 3; i++) {
         temp1[i] = op1[i];
         temp2[i] = op2[i];
     }
     if (op1[0] > 127)
         Negl(temp1);
     if (op2[0] > 127)
         Negl(temp2);
     while ((temp2[0] != 0) || (temp2[1] != 0) || (temp2[2] != 0)) {
         if (temp2[2] & 1)
             Addl(res, temp1, res);
         Shift(temp2, 1, 1);
         Shift(temp1, 1, 0);
     }
     if (op1[0] > 127 && op2[0] < 128 || op1[0] < 128 && op2[0] > 127)
         Negl(res);
     for (i = 0; i < 3; i++)
         result[i] = res[i];
} /*Mull*/

/******************************************************************/

void Divl(BYTE *op1, BYTE *op2, BYTE *result)                  /*divide*/
{
  WORD temp1, temp2, a, res;
  int  i, count;

     if (op2[0] == 0 && op2[1] == 0 && op2[2] == 0) {
         SICError(1); /*division by zero*/
     } else {
         for (i = 0; i < 3; i++)
             res[i] = 0;
         for (i = 0; i < 3; i++) {
             temp1[i] = op1[i];
             temp2[i] = op2[i];
             a[i] = Word1[i];
         }
         if (op1[0] > 127)
             Negl(temp1);
         if (op2[0] > 127)
             Negl(temp2);
         count = 0;
         while (temp2[0] <= temp1[0] && temp2[0] < 64) {
             Shift(temp2, 1, 0);
             count++;
         }
         Shift(a, count, 0);
         while (temp2[0] != 0 || temp2[1] != 0 || temp2[2] != 0) {
             Subl(temp1, temp2, temp1);
             if (temp1[0] > 127)
                 Addl(temp1, temp2, temp1);
             else
                 Addl(res, a, res);
             Shift(a, 1, 1);
             Shift(temp2, 1, 1);
         }
         if ((op1[0] > 127 && op2[0] < 128) || (op1[0] < 128 && op2[0] > 127))
             Negl(res);
         for (i = 0; i < 3; i++)
             result[i] = res[i];
     }
} /*Divl*/

/******************************************************************/

void Compl (BYTE *op1, BYTE *op2)                              /*compare*/
{
  /* This procedure compares the values of op1 and op2, and sets the
     condition code to indicate the result. */

     Status[2] &= 0x3f;         /* clear the condition code */
     if (op1[0] > 127 && op2[0] < 128)
         Status[2] |= (LT << 6);
     else if (op1[0] < 128 && op2[0] > 127)
         Status[2] |= (GT << 6);
     else if (op1[0] == op2[0] && op1[1] == op2[1] && op1[2] == op2[2])
         Status[2] |= (EQ << 6);
     else if (op1[0] > op2[0] || op1[0] == op2[0] && op1[1] > op2[1]
                 || op1[0] == op2[0] && op1[1] == op2[1] && op1[2] > op2[2])
         Status[2] |= (GT << 6);
     else
         Status[2] |= (LT << 6);
} /*Compl*/

/******************************************************************/

void Load(int opcode, WORD targaddr, BOOLEAN indir, BOOLEAN immed,
             int *regno, WORD data, ADDRESS *opaddr)
{
  /* Handles the instructions  LDA, LDX, LDL, LDCH, LDB, LDS, LDT */

  int i;

     GetData(opcode, targaddr, indir, immed, data, opaddr);
     switch (opcode) {
         case 0:    /* LDA */
                 *regno = 0;
                 break;
         case 4:    /* LDX */
                 *regno = 1;
                 break;
         case 8:    /* LDL */
                 *regno = 2;
                 break;
         case 80:   /* LDCH */
                 break;
         case 104:  /* LDB */
                 *regno = 3;
                 break;
         case 108:  /* LDS */
                 *regno = 4;
                 break;
         case 116:  /* LDT */
                 *regno = 5;
                 break;
     }
     if (opcode == 80)  /* LDCH */
         if (immed)
             Registers[0][2] = data[2];
         else
             Registers[0][2] = data[0];
     else
         for (i = 0; i < 3; i++)
             Registers[*regno][i] = data[i];
} /*Load*/

/******************************************************************/

void Store(int opcode, WORD targaddr, BOOLEAN indir, BOOLEAN immed,
              int *regno, ADDRESS *opaddr)
{
  /* Handles the instructions: STA, STX, STL, STCH, STB, STS, STT */

  int i;

     if (immed) {
         SICError(8);  /* store immediate not allowed */
     } else {
         GetAddr(opcode, targaddr, indir, opaddr);
         switch (opcode) {
             case 12:  /* STA */
                     *regno = 0;
                     break;
             case 16:  /* STX */
                     *regno = 1;
                     break;
             case 20:  /* STL */
                     *regno = 2;
                     break;
             case 84:  /* STCH */
                     break;
             case 120: /* STB */
                     *regno = 3;
                     break;
             case 124: /* STS */
                     *regno = 4;
                     break;
             case 132: /* STT */
                     *regno = 5;
                     break;
         }
         if (opcode == 84)  /* STCH */
             Memory[*opaddr] = Registers[0][2];
         else
             for (i = 0; i < 3; i++)
                 Memory[*opaddr + i] = Registers[*regno][i];
     }
} /*Store*/

/******************************************************************/

void Jump(int opcode, WORD targaddr, BOOLEAN indir, BOOLEAN immed,
             ADDRESS *opaddr)
{
  /* Handles the instructions  JEQ, JGT, JLT, J, JSUB, RSUB */
  int i;
  ADDRESS temppc;
  BOOLEAN jumpc;

     if (immed) {
         SICError(8);  /* jump immediate not allowed */
     } else {
         if (opcode == 76) {   /*rsub*/
             temppc = 0;
             if ((Registers[2][0] == 0xff) && (Registers[2][1] == 0xff)
                        && (Registers[2][2] == 0xff))
                 ERROR = TRUE;
             i = 0;
             while (!ERROR && i < 3)
                 if (MSIZE < 256 * temppc + Registers[2][i]) {
                     SICError(3);  /* address out of range */
                 } else {
                     temppc = 256 * temppc + Registers[2][i];
                     i++;
                 }
             if (!ERROR)
                 PC = temppc;
         } else {
             jumpc = FALSE;
             switch  (opcode) {
                 case 48:  /* JEQ */
                         if (((Status[2] >> 6) & 3) == EQ)
                             jumpc = TRUE;
                         break;
                 case 52:  /* JGT */
                         if (((Status[2] >> 6) & 3) == GT)
                             jumpc = TRUE;
                         break;
                 case 56:  /* JLT */
                         if (((Status[2] >> 6) & 3) == LT)
                             jumpc = TRUE;
                         break;
                 case 60:  /* J */
                 case 72:  /* JSUB */
                         jumpc = TRUE;
                         break;
             }
             if (jumpc)
                 GetAddr(opcode, targaddr, indir, opaddr);
             if (opcode == 72) {   /* JSUB */
                 Registers[2][2] = PC & 255;
                 PC /= 256;
                 Registers[2][1] = PC & 255;
                 Registers[2][0] = PC / 256;
             }
             if (jumpc && !ERROR)
                 PC = *opaddr;
         }
     }
} /* Jump */

/******************************************************************/

void Arith(int opcode, WORD targaddr, BOOLEAN indir, BOOLEAN immed, WORD data,
              ADDRESS *opaddr)
{
  /* Handles instructions  ADD, SUB, MUL, DIV, COMP, TIX */

  int i;
  WORD result;

      GetData(opcode, targaddr, indir, immed, data, opaddr);
      switch (opcode) {
          case 24:  /* ADD */
                  Addl(Registers[0],data,Registers[0]);
                  break;
          case 28:  /* SUB */
                  Subl(Registers[0],data,Registers[0]);
                  break;
          case 32:  /* MUL */
                  Mull(Registers[0],data,Registers[0]);
                  break;
          case 36:  /* DIV */
                  Divl(Registers[0],data,Registers[0]);
                  break;
          case 40:  /* COMP */
                  Compl(Registers[0],data);
                  break;
          case 44:  /* TIX */
                  Addl(Registers[1],Word1,Registers[1]);
                  Compl(Registers[1],data);
                  break;
      } /*case*/
} /*Arith*/

/******************************************************************/

void Logic(int opcode, WORD targaddr, BOOLEAN indir, BOOLEAN immed, WORD data,
              ADDRESS *opaddr)
{
  /* Handles the instructions  AND, OR */

  int i;
  WORD temp;

      GetData(opcode, targaddr, indir, immed, data, opaddr);
      for (i = 0; i < 3; i++) {
          temp[i] = Registers[0][i];
          Registers[0][i] = 0;
      }
      for (i = 1; i < 24; i++) {
          if (opcode == 64 && (temp[0] & 1) && (data[0] & 1))  /* AND */
              Registers[0][0]++;
          if (opcode == 68 && ((temp[0] & 1) || (data[0] & 1)))  /* OR */
              Registers[0][0]++;
          Shift(temp, 1, 0);
          Shift(data, 1, 0);
          Shift(Registers[0], 1, 0);
      }
} /*Logic*/

/******************************************************************/

void CharIO(int opcode, WORD targaddr, BOOLEAN indir, BOOLEAN immed,
               WORD data, ADDRESS *opaddr)
{
  /* Handles the instructions  RD, WD, TD */

  char c;
  int Devcode;            /* holds I/O device number */

      GetData(opcode, targaddr, indir, immed, data, opaddr);
      if (immed)
          data[0] = data[2];
      if (data[0] > 240)
          Devcode = data[0] - 240;
      else
          Devcode = data[0];
      Devcode--;                /* adjust for arrays starting at 0 */

      if (opcode == 224) {  /* TD */
          if (Devcode >= 0 && Devcode < 6)
              if (Wait[Devcode] == 0) {
                  Status[2] &= 0x3f;
                  Status[2] |= (LT << 6);
                  Wait[Devcode] = ((Devcode + 1) & 3) + 2;
              } else {
                  Status[2] &= 0x3f;
                  Status[2] |= (EQ << 6);
                  Wait[Devcode]--;
              }
          else {
              SICError(9);  /* unsupported I/O device */
          }
      }

      if (opcode == 216) {  /* RD */
          if (Devcode < 0 || Devcode > 2) {
              SICError(11);  /* device not available for read */
          } else {
              if (Wait[Devcode] != ((Devcode + 1) & 3) + 2) {
                  SICError(10);  /* device not ready for I/O*/
              } else
                  Wait[Devcode]--;
          }
          if (!ERROR)
              if (!Init[Devcode]) {
                  if ((Dev[Devcode] = fopen(SICFile[Devcode],"r")) == NULL) {
                      printf("cannot open file %s\n", SICFile[Devcode]);
                      exit(1);
                  }
                  Init[Devcode] = TRUE;
              }
              if (EndFile[Devcode]) {
                  SICError(13);  /* attempt to read past end of file */
              } else
                  if (feof(Dev[Devcode])) {
                      EndFile[Devcode] = TRUE;
                      Registers[0][2] = 4;
                  } else
                      if (SICEoln(Dev[Devcode])) {
                          Registers[0][2] = 0;
                          fscanf(Dev[Devcode], "%*[^\n]");
                          fgetc(Dev[Devcode]);
                      } else {
                          c = fgetc(Dev[Devcode]);
                          if (c == '\n')
                          c = ' ';
                          Registers[0][2] = InTab[c];
                      }
      }

      if (opcode == 220) {   /* WD */
          if (Devcode < 3 || Devcode > 5) {
              SICError(12);  /* device not available for write */
          } else {
              if (Wait[Devcode] != ((Devcode + 1) & 3) + 2) {
                  SICError(10);  /* device not ready for I/O */
              } else
                  Wait[Devcode]--;
          }
          if (!ERROR) {
              if (!Init[Devcode]) {
                  if ((Dev[Devcode] = fopen(SICFile[Devcode],"w")) == NULL) {
                      printf("cannot open file %s\n", SICFile[Devcode]);
                      exit(1);
                  }
                  Init[Devcode] = TRUE;
              }
              if (Registers[0][2] == 0)
                  fputc('\n', Dev[Devcode]);
              else
                  fputc(OutTab[Registers[0][2]], Dev[Devcode]);
          }
      }
} /*CharIO*/

/******************************************************************/

void RegReg(int opcode, int reg1, int reg2)
{
  /* Handles the instructions  ADDR, SUBR, MULR, DIVR, COMPR, TIXR */

      if (reg1 > 5 || reg2 > 5 && opcode != 184 /*tixr*/ ) {
          SICError(4);  /* illegal register number */
      } else
          switch (opcode) {
              case 144:
                       Addl(Registers[reg2],Registers[reg1],Registers[reg2]);
                       break;
              case 148:
                       Subl(Registers[reg2],Registers[reg1],Registers[reg2]);
                       break;
              case 152:
                       Mull(Registers[reg2],Registers[reg1],Registers[reg2]);
                       break;
              case 156:
                       Divl(Registers[reg2],Registers[reg1],Registers[reg2]);
                       break;
              case 160:
                       Compl(Registers[reg1],Registers[reg2]);
                       break;
              case 184:                           /* TIXR */
                       Addl(Registers[0],Word1,Registers[0]);
                       Compl(Registers[0],Registers[reg1]);
          } /*case*/
} /*RegReg*/

/********************/

void RegMan(int opcode, int reg1, int reg2)
{
  /* Handles the instructions  SHIFTL, SHIFTR, RMO, CLEAR */
  int i,stype;

      if (reg1 > 5 || opcode == 172 /* RMO */  && reg2 > 5) {
          SICError(4);  /* illegal register number */
      } else
          if (opcode == 180)                      /* CLEAR */
              for (i = 0; i < 3; i++)
                  Registers[reg1][i] = 0;
          else
              if (opcode == 172)                  /* RMO */
                  for (i = 0; i < 3; i++)
                      Registers[reg2][i] = Registers[reg1][i];
              else {                              /* SHIFTL, SHIFTR */
                  if (opcode == 164)
                      stype = 0;
                  else
                      stype = 1;
                  Shift(Registers[reg1], reg2 + 1, stype);
              }
} /*RegMan*/

/******************************************************************/

void SICExec(int opcode, int reg1, int reg2, WORD targaddr, BOOLEAN indir,
             BOOLEAN immed)
{
  /* This procedure simulates the execution of the machine
     instruction that was decoded by the procedure 'SICFetch'.
     It calls an internal procedure, depending upon the value
     of opcode, to execute the instruction. */

  int i, regno;
  WORD data;
  ADDRESS opaddr;

     switch (opcode) {
         case 0:                /* LDA, LDX, LDL, LDCH, LDB, LDS, LDT */
         case 4:
         case 8:
         case 80:
         case 104:
         case 108:
         case 116:
                 Load(opcode, targaddr, indir, immed, &regno, data, &opaddr);
                 break;

         case 12:
         case 16:
         case 20:
         case 84:
         case 120:
         case 124:
         case 132:   /* STA, STX, STL, STCH, STB, STS, STT */
                 Store(opcode, targaddr, indir, immed, &regno, &opaddr);
                 break;

         case 48:
         case 52:
         case 56:
         case 60:
         case 72:
         case 76:   /* JEQ, JGT, JLT, J, JSUB, RSUB */
                 Jump(opcode, targaddr, indir, immed, &opaddr);
                 break;

         case 24:
         case 28:
         case 32:
         case 36:
         case 40:
         case 44:   /* ADD, SUB, MUL, DIV, COMP, TIX */
                 Arith(opcode, targaddr, indir, immed, data, &opaddr);
                 break;

         case 64:
         case 68:   /* AND, OR */
                 Logic(opcode, targaddr, indir, immed, data, &opaddr);
                 break;

         case 216:
         case 220:
         case 224:   /* RD, WD, TD*/
                 CharIO(opcode, targaddr, indir, immed, data, &opaddr);
                 break;

         case 144:
         case 148:
         case 152:
         case 156:
         case 160:
         case 184:   /* ADDR, SUBR, MULR, DIVR, COMPR, TIXR */
                 RegReg(opcode, reg1, reg2);
                 break;

         case 164:
         case 168:
         case 172:
         case 180:   /* SHIFTL, SHIFTR, RMO, CLEAR */
                 RegMan(opcode, reg1, reg2);
                 break;
     }
} /* SICExec */

/******************************************************************/

void SICStart()
{
  /* This procedure reads 128 bytes of data from DevBoot (the bootstrap device)
     and enters it into memory beginning at address 0. The bootstrap data
     is stored on the file as four lines of 32 characters each; each pair
     of characters gives the hexadecimal representation of one byte of data.
     If the bootstrap is shorter than 128 bytes, it must be extended to 128
     bytes by padding it with legal hex characters such as '0000...'. */

  int i, k, r, l;
  char chl, chr;
  BOOLEAN err1;

     err1 = FALSE;
     if ((DevBoot = fopen("dev00","r")) == NULL) {
         printf("cannot open boot file DEV00\n");
         exit(1);
     }
     for (k = 0; k <= 3; k++) {
         for (i = 0; i <= 31; i++)
             if (!err1) {
                 if (feof(DevBoot))
                     chl = ' ';
                 else {
                     chl = fgetc(DevBoot);
                     if (chl == '\n')
                         chl = ' ';
                 }
                 if (feof(DevBoot))
                     chr = ' ';
                 else {
                     chr = fgetc(DevBoot);
                     if (chl == '\n')
                         chl = ' ';
                 }
                 l = chl - 48;
                 r = chr - 48;
                 if (l < 0 || r < 0) {
                     SICError(13);   /*illegal bootstrap data*/
                     err1 = TRUE;
                 } else
                     Memory[32 * k + i] = 16 * l + r;
             }
         if (!feof(DevBoot)) {
             fscanf(DevBoot,"%*[^\n]");
             fgetc(DevBoot);  /* need to grab the CR */
         }
     }
     if (DevBoot != NULL)
         fclose(DevBoot);
     DevBoot = NULL;   /* only want to read once */
} /*SICStart*/

/******************************************************************/

void DecMode(BOOLEAN *err3, BOOLEAN *indir, BOOLEAN *immed, BOOLEAN *index,
                BOOLEAN *brel, BOOLEAN *PCrel, BOOLEAN *SICstd, BOOLEAN fmt3,
                BOOLEAN fmt4)
{
  /* This procedure decodes and validates the addressing mode
     bits for a format 3 or 4 instruction. it sets the variables
     indir, immed, index, brel, PCrel, and SICstd to indicate
     the proper addressing mode. */

  int flags, modes;

     flags = Memory[PC] & 3;
     switch (flags) {
         case 0: *indir = FALSE;
                 *immed = FALSE;
                 *SICstd = TRUE;
                 break;
         case 1: *indir = FALSE;
                 *immed = TRUE;
                 *SICstd = FALSE;
                 break;
         case 2: *indir = TRUE;
                 *immed = FALSE;
                 *SICstd = FALSE;
                 break;
         case 3: *indir = FALSE;
                 *immed = FALSE;
                 *SICstd = FALSE;
                 break;
     }
     modes = Memory[PC + 1] / 32;
     if (fmt3)
         if (*SICstd)
             *err3 = FALSE;
         else if (!*indir && !*immed) {
             if (modes == 3 || modes == 7)
                 *err3 = TRUE;
             else
                 *err3 = FALSE;
         } else {
             if (modes > 2)
                 *err3 = TRUE;
             else
                 *err3 = FALSE;
         }
     if (fmt4)
         if (!*indir && !*immed && !*SICstd)
             if (modes == 0 || modes == 4)
                 *err3 = FALSE;
             else
                 *err3 = TRUE;
         else
             if (modes == 0)
                 *err3 = FALSE;
             else
                 *err3 = TRUE;
     if (modes > 3)
         *index = TRUE;
     else
         *index = FALSE;
     if (*SICstd) {
         *brel = FALSE;
         *PCrel = FALSE;
     } else {
         if (modes & 1 == 1)
             *PCrel = TRUE;
         else
             *PCrel = FALSE;
         if (modes & 3 > 1)
             *brel = TRUE;
         else
             *brel = FALSE;
     }
} /*DecMode*/

/******************************************************************/

void DecAddr(WORD targaddr, BOOLEAN *immed, BOOLEAN *index, BOOLEAN *brel,
                 BOOLEAN *PCrel, BOOLEAN *SICstd, BOOLEAN fmt3, BOOLEAN fmt4,
                 ADDRESS newPC)
{
  /* This procedure decodes the 'disp' and 'addr' fields in format 3
     and format 4 instructions. It sets the variable disp
     to indicate the displacement specified (format 3), and
     targaddr to indicate the target address specified
     (format 3 or 4). */

  int i;
  ADDRESS PCtemp;
  WORD disp, PCword;

     if (fmt3) {         /*decode disp*/
         if (*SICstd) {
             disp[0] = 0;
             disp[1] = Memory[PC + 1] & 127;
             disp[2] = Memory[PC + 2];
         } else {
             disp[0] = 0;
             disp[1] = Memory[PC + 1] & 15;
             disp[2] = Memory[PC + 2];
             if (*PCrel || *immed)
                 if (disp[1] > 7) {
                     disp[1] += 240;
                     disp[0] = 255;
                 }
         }
         if (*SICstd)
             for (i = 0; i < 3; i++)
                 targaddr[i] = disp[i];
         else {
             if (*brel)
                 Addl(Registers[3], disp, targaddr);
             else
                 if (*PCrel) {
                     PCtemp = newPC;
                     for (i = 1; i <= 3; i++) {
                         PCword[3 - i] = PCtemp & 255;
                         PCtemp /= 256;
                     }
                     Addl(PCword, disp, targaddr);
                 } else
                     for (i = 0; i <= 3; i++)
                         targaddr[i] = disp[i];
         }
     }
     if (fmt4) {         /*decode addr*/
         targaddr[0] = Memory[PC + 1] % 15;
         targaddr[1] = Memory[PC + 2];
         targaddr[2] = Memory[PC + 3];
     }
     if (*index)
         Addl(targaddr, Registers[1], targaddr);
} /*DecAddr*/

/******************************************************************/

void SICFetch(int *opcode, int *reg1, int *reg2, WORD targaddr, BOOLEAN *indir,
              BOOLEAN *immed, BOOLEAN *index, BOOLEAN *brel, BOOLEAN *PCrel,
              BOOLEAN *SICstd)
{
  /* This procedure fetches the next machine instruction from the
     location indicated by PC, decodes the instruction, and advances
     PC to the next instruction. If an error is detected, it sets
     the appropriate error flags.

     When an instruction is fetched for execution, the following
     variables are set. The values of these variables are used
     in executing the instruction.
          opcode -- machine operation code
          fmt1, fmt2, fmt3, fmt4 -- indicate instruction format
          reg1, reg2 -- registers specified (format 2)
          disp -- displacement (format 3)
          indir, immed, index, brel, PCrel, SICstd -- indicate addressing
               mode (format 3 and 4)

     Routine DecMode is called to decode and validate the addressing
     mode bits in a format 3 or 4 instruction. Routine DecAddr is called
     to decode the displacement field in a format 3 instruction or the
     address field in a format 4 instruction and convert these values to
     numeric. */

  int i, flags, modes;
  BOOLEAN fmt1, fmt2, fmt3, fmt4, err1, err2, err3;
  ADDRESS newPC;

     fmt1 = FALSE;
     fmt2 = FALSE;
     fmt3 = FALSE;
     fmt4 = FALSE;
     err1 = FALSE;
     err2 = FALSE;
     err3 = FALSE;
     /* check for valid opcode */
     *opcode = (Memory[PC] / 4) * 4;
     if (*opcode >= 88 && *opcode <= 100 || *opcode == 112
             || *opcode == 128 || *opcode == 136 || *opcode == 176
             || *opcode >= 192 && *opcode <= 200 || *opcode == 208
             || *opcode == 212 || *opcode >= 228 && *opcode <= 248)
         err1 = TRUE;
     if (*opcode == 140 || *opcode == 188 || *opcode == 204 || *opcode == 252)
         err2 = TRUE;
     /* determine instruction format */
     if (*opcode <= 140 || *opcode >= 208 && *opcode <= 236)
         fmt3 = TRUE;
     else if (*opcode <= 188)
         fmt2 = TRUE;
     else
         fmt1 = TRUE;
     if (fmt3 && ((Memory[PC + 1] / 16) & 1) == 1 && (Memory[PC] & 3) != 0) {
         fmt3 = FALSE;
         fmt4 = TRUE;
     }
     if (fmt1)
         newPC = PC + 1;
     else if (fmt2)
         newPC = PC + 2;
     else if (fmt3)
         newPC = PC + 3;
     else
         newPC = PC + 4;
     if (newPC > (MSIZE - 2)) {
         SICError(3); /* address out of range */
     }
     if (fmt2 && !ERROR) {  /* decode register numbers */
         *reg1 = Memory[PC + 1] / 16;
         *reg2 = Memory[PC + 1] & 15;
     }
     if ((fmt3 || fmt4) && !ERROR) {
         DecMode(&err3, indir, immed, index, brel, PCrel, SICstd, fmt3, fmt4);
         DecAddr(targaddr, immed, index, brel, PCrel, SICstd, fmt3, fmt4, newPC);
     }
     if (!XE && !err2) {
         if (!fmt3)
             err1 = TRUE;
         if (fmt3 && !*SICstd)
             err3 = TRUE;
     }
     if (err1 || err2 || err3) {
         if (err1)
             SICError(5);  /* unsupported machine instruction */
         if (err2)
             SICError(6);  /* illegal machine instruction */
         if (err3)
             SICError(7);  /* illegal addressing mode */
     } else
         PC = newPC;
} /*SICFetch*/

/******************************************************************/

void SICRun(ADDRESS *TempPC, BOOLEAN SingleStep)
{
  /* This procedure contains the main loop for simulating the execution
     of machine instructions. It calls the procedures 'SICFetch' and 'SICExec'
     to fetch and execute each instruction in turn; it also checks for
     breakpoints and instruction counts, issuing appropriate messages
     to the user. */

   int i;
   BOOLEAN running;
   int opcode, reg1, reg2;                 /*current instruction*/
   WORD disp, targaddr, addr;
   BOOLEAN indir, immed, index, brel, PCrel, SICstd,
           fmt1, fmt2, fmt3, fmt4;

     running = TRUE;
     ERROR = FALSE;
     if (*TempPC > MSIZE) {
         SICError(3);      /* invalid address specified */
     }
     PC = *TempPC;
     while (running && !ERROR) {
         SICFetch(&opcode, &reg1, &reg2, targaddr, &indir, &immed, &index,
                &brel, &PCrel, &SICstd);
         if (!ERROR)
             SICExec(opcode, reg1, reg2, targaddr, indir, immed);
         if (SingleStep) {
             running = FALSE;
             printf("\nStepped to PC = %x\n", PC);
         }
     }
     *TempPC = PC;
} /* SICRun */

/******************************************************************/

void SICInit()
{
  /* This procedure is called at the beginning of the simulation
     to set up initial values. InTab and OutTab are set to reflect
     the character collating sequence of the host machine (see notes
     on installing the simulator). */

  int i, j;
  long loc;

#if 0
     if ((Log = fopen("log","w")) == NULL) {
  printf("cannot open file LOG\n");
  exit(1);
     }
#endif
     for (i = 0; i <= 255; i++)
         InTab[i] = i;
  /* +++++ initialization for non-ascii character sets goes here +++++ */
     for (i = 0; i <= 255; i++)
         OutTab[InTab[i]] = i;

     for (i = 0; i < 6; i++) {      /* set up I/O device status */
         Init[i] = FALSE;
         Wait[i] = 0;
         EndFile[i] = FALSE;
     }
     Word1[0] = 0;                  /* define a word containing value 1 */
     Word1[1] = 0;
     Word1[2] = 1;
     for (loc = 0; loc < MSIZE; loc++) /* initialize memory to hex 'ff' */
         Memory[loc] = 255;
     for (i = 0; i < 6; i++)        /* initialize registers to hex 'ff' */
         for (j = 0; j < 3; j++)
             Registers[i][j] = 255;
     PC = 0;
     for (i = 0; i < 3; i++)        /* initialize status word */
         Status[i] = 0;
     Msg[0] = strdup(" ");
     Msg[1] = strdup("Division by zero");
     Msg[2] = strdup("Integer overflow");
     Msg[3] = strdup("Invalid address");
     Msg[4] = strdup("Invalid register");
     Msg[5] = strdup("Unsupported instruction");
     Msg[6] = strdup("Illegal instruction");
     Msg[7] = strdup("Illegal addressing mode");
     Msg[8] = strdup("Immediate not allowed");
     Msg[9] = strdup("Unsupported I/O device");
     Msg[10] = strdup("I/O device not ready");
     Msg[11] = strdup("Device not open for read");
     Msg[12] = strdup("Device not open for write");
     Msg[13] = strdup("End of file reached");
     for (i = 14; i < 16; i++)
         Msg[i] = strdup(" ");
} /* SICInit */
