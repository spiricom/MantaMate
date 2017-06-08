#ifndef TYPE_H
#define TYPE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  #include <sys/types.h>

  /* 
   * Types
   */
  #if !AIX
  typedef unsigned char  uchar;
  #endif

  #if HPUX || __APPLE__
  typedef unsigned long  ulong;
  #endif

  typedef short          wchar;

  /* 
   * Constants
   */
  #define PATH_SIZE               10
  #define USAGE_TAB_SIZE          50
  #define MAX_REPORT             300

  #define REPORT_DSC_SIZE       6144

  /* 
   * Items
   * -------------------------------------------------------------------------- */
  #define SIZE_0                0x00
  #define SIZE_1                0x01
  #define SIZE_2                0x02
  #define SIZE_4                0x03
  #define SIZE_MASK             0x03

  #define TYPE_MAIN             0x00
  #define TYPE_GLOBAL           0x04
  #define TYPE_LOCAL            0x08
  #define TYPE_MASK             0x0C

  /* Main items */
  #define ITEM_COLLECTION       0xA0
  #define ITEM_END_COLLECTION   0xC0
  #define ITEM_FEATURE          0xB0
  #define ITEM_INPUT            0x80
  #define ITEM_OUTPUT           0x90

  /* Global items */
  #define ITEM_UPAGE            0x04
  #define ITEM_LOG_MIN          0x14
  #define ITEM_LOG_MAX          0x24
  #define ITEM_PHY_MIN          0x34
  #define ITEM_PHY_MAX          0x44
  #define ITEM_UNIT_EXP         0x54
  #define ITEM_UNIT             0x64
  #define ITEM_REP_SIZE         0x74
  #define ITEM_REP_ID           0x84
  #define ITEM_REP_COUNT        0x94

  /* Local items */
  #define ITEM_USAGE            0x08
  #define ITEM_STRING           0x78

  /* Long item */
  #define ITEM_LONG       0xFC

  #define ITEM_MASK             0xFC

  /* Attribute Flags */
  #define ATTR_DATA_CST         0x01
  #define ATTR_NVOL_VOL         0x80

  typedef struct
  {
    ushort UPage;
    ushort Usage;
  } HIDNode;

  typedef struct
  {
    uchar   Size;
    HIDNode Node[PATH_SIZE];
  } HIDPath;

  typedef struct
  {
    long    Value;
    HIDPath Path;
    uchar   ReportID;
    uchar   Offset;
    uchar   Size;
    uchar   Type;
    uchar   Attribute;
    ulong   Unit;
    char    UnitExp;
    long    LogMin;
    long    LogMax;
    long    PhyMin;
    long    PhyMax;
  } HIDData;

  /* -------------------------------------------------------------------------- */
  typedef struct
  {
    uchar   ReportDesc[REPORT_DSC_SIZE];
    ushort  ReportDescSize;
    ushort  Pos;
    uchar   Item;
    long    Value;
    HIDData Data;
    uchar   OffsetTab[MAX_REPORT][3];
    uchar   ReportCount;
    uchar   Count;
    ushort  UPage;
    HIDNode UsageTab[USAGE_TAB_SIZE];
    uchar   UsageSize;
    uchar   nObject;
    uchar   nReport;
  } HIDParser;

  #ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif
