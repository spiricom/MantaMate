
/* -------------------------------------------------------------------------- */

#ifndef HIDPARS_H
#define HIDPARS_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  #include "hidtypes.h"

  /*
   * HIDParse
   * -------------------------------------------------------------------------- */
  int HIDParse(HIDParser* pParser, HIDData* pData);

  /*
   * ResetParser
   * -------------------------------------------------------------------------- */
  void ResetParser(HIDParser* pParser);

  /*
   * FindObject
   * -------------------------------------------------------------------------- */
  int FindObject(HIDParser* pParser, HIDData* pData);

  /*
   * GetValue
   * -------------------------------------------------------------------------- */
  void GetValue(const uchar* Buf, HIDData* pData);

  /*
   * SetValue
   * -------------------------------------------------------------------------- */
  void SetValue(const HIDData* pData, uchar* Buf);

  /*
   * GetReportOffset
   * -------------------------------------------------------------------------- */
  uchar* GetReportOffset(HIDParser* pParser, const uchar ReportID,
			 const uchar ReportType);

  #ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif
