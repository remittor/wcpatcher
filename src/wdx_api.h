#pragma once

// Contents of file contplug.h version 2.10
 
#define ft_nomorefields      0
#define ft_numeric_32        1
#define ft_numeric_64        2
#define ft_numeric_floating  3
#define ft_date              4
#define ft_time              5
#define ft_boolean           6
#define ft_multiplechoice    7
#define ft_string            8
#define ft_fulltext          9
#define ft_datetime          10
#define ft_stringw           11
#define ft_fulltextw         12
 
#define ft_comparecontent    100
 
// for ContentGetValue
#define ft_nosuchfield       -1  // error, invalid field number given
#define ft_fileerror         -2  // file i/o error
#define ft_fieldempty        -3  // field valid, but empty
#define ft_ondemand          -4  // field will be retrieved only when user presses <SPACEBAR>
#define ft_notsupported      -5  // function not supported
#define ft_setcancel         -6  // user clicked cancel in field editor
#define ft_delayed            0  // field takes a long time to extract -> try again in background
 
// for ContentSetValue
#define ft_setsuccess         0  // setting of the attribute succeeded
 
// for ContentGetSupportedFieldFlags
typedef enum cont_subst : BYTE {
  cont_size     = 1,
  cont_datetime = 2,
  cont_date     = 3,
  cont_time     = 4,
  cont_attributes = 5,
  cont_attributestr = 6,
  cont_passthrough_size_float = 7,
} cont_subst;

typedef struct {
  BYTE   edit      : 1;
  BYTE   subst     : 3;   /* see cont_subst */
  BYTE   fieldedit : 1;
  BYTE   reserved  : 3;
} tcContFlags_t;

#define contflags_edit                   0x01        /* The plugin allows to edit (modify) this field. */
#define contflags_substsize              (cont_size << 1)
#define contflags_substdatetime          (cont_datetime << 1)
#define contflags_substdate              (cont_date << 1)
#define contflags_substtime              (cont_time << 1)
#define contflags_substattributes        (cont_attributes << 1)
#define contflags_substattributestr      (cont_attributestr << 1)
#define contflags_passthrough_size_float (cont_passthrough_size_float << 1)
#define contflags_substmask              0x0E
#define contflags_fieldedit              0x10

#define contst_readnewdir        0x01
#define contst_refreshpressed    0x02
#define contst_showhint          0x04
 
#define setflags_first_attribute 0x01  // First attribute of this file
#define setflags_last_attribute  0x02  // Last attribute of this file
#define setflags_only_date       0x04  // Only set the date of the datetime value!
 
#define editflags_initialize     0x01  // The data passed to the plugin may be used to initialize the edit dialog
 
#define CONTENT_DELAYIFSLOW 1  // ContentGetValue called in foreground
#define CONTENT_PASSTHROUGH 2  // If requested via contflags_passthrough_size_float: The size
                               // is passed in as floating value, TC expects correct value
                               // from the given units value, and optionally a text string

#pragma pack(push, 1)
 
typedef struct {
  int   size;
  DWORD PluginInterfaceVersionLow;
  DWORD PluginInterfaceVersionHi;
  char  DefaultIniName[MAX_PATH];
} ContentDefaultParamStruct;
 
typedef struct {
  WORD wYear;
  WORD wMonth;
  WORD wDay;
} tdateformat,*pdateformat;
 
typedef struct {
  WORD wHour;
  WORD wMinute;
  WORD wSecond;
} ttimeformat,*ptimeformat;
 
typedef struct {
  INT64    filesize1;
  INT64    filesize2;
  FILETIME filetime1;
  FILETIME filetime2;
  DWORD    attr1;
  DWORD    attr2;
} FileDetailsStruct;

#pragma pack(pop)
 
typedef int (__stdcall * PROGRESSCALLBACKPROC)(int nextblockdata);

int  __stdcall ContentGetDetectString(char * DetectString, int maxlen);

int  __stdcall ContentGetSupportedField(int FieldIndex, char * FieldName, char * Units, int maxlen);

int  __stdcall ContentGetValue(char * FileName, int FieldIndex, int UnitIndex, void * FieldValue, int maxlen, int flags);
int  __stdcall ContentGetValueW(LPCWSTR FileName, int FieldIndex, int UnitIndex, void * FieldValue, int maxlen, int flags);

void __stdcall ContentSetDefaultParams(ContentDefaultParamStruct * dps);

void __stdcall ContentPluginUnloading(void);

void __stdcall ContentStopGetValue(char * FileName);
void __stdcall ContentStopGetValueW(LPCWSTR FileName);

int  __stdcall ContentGetDefaultSortOrder(int FieldIndex);

int  __stdcall ContentGetSupportedFieldFlags(int FieldIndex);

int  __stdcall ContentSetValue(char * FileName, int FieldIndex, int UnitIndex, int FieldType, void * FieldValue, int flags);
int  __stdcall ContentSetValueW(LPCWSTR FileName, int FieldIndex, int UnitIndex, int FieldType, void * FieldValue, int flags);

int  __stdcall ContentEditValue(HWND ParentWin, int FieldIndex, int UnitIndex, int FieldType, void * FieldValue, int maxlen, int flags, LPCSTR langidentifier);

void __stdcall ContentSendStateInformation(int state, LPCSTR path);
void __stdcall ContentSendStateInformationW(int state, LPCWSTR path);

int  __stdcall ContentCompareFiles(PROGRESSCALLBACKPROC progresscallback, int compareindex, 
                                   char * filename1, char * filename2, FileDetailsStruct * filedetails);
int  __stdcall ContentCompareFilesW(PROGRESSCALLBACKPROC progresscallback, int compareindex,
                                    LPCWSTR filename1, LPCWSTR filename2, FileDetailsStruct * filedetails);

