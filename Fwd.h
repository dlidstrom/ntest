// $Id$
//! @file
//! Brief.
//! @author Daniel Lidstrom <daniel.lidstrom@sbg.se>
//! @date   2009-11-30 17:25
//! @ingroup
//!

#if !defined(FWD_H__20091130T1725)
#define FWD_H__20091130T1725

#include <boost/tr1/memory.hpp>                   // shared_ptr
#include <vector>

class CBook;
typedef std::tr1::shared_ptr<CBook> CBookPtr;
class CBookData;
class CBoni;
class CCache;
class CCalcParams;
typedef std::tr1::shared_ptr<CCalcParams> CCalcParamsPtr;
class CCalcParamsFixedHeight;
typedef std::tr1::shared_ptr<CCalcParamsFixedHeight> CCalcParamsFixedHeightPtr;
class CComputerDefaults;
class CEvaluator;
class CGame;
class CMove;
class CMoves;
class CMoveValue;
class CMPCStats;
class CMVK;
class COsGame;
class CPlayer;
typedef std::tr1::shared_ptr<CPlayer> CPlayerPtr;
class CPlayerComputer;
typedef std::tr1::shared_ptr<CPlayerComputer> CPlayerComputerPtr;
class CQPosition;
class CSavedGame;
class CSearchInfo;
struct CWindow;
struct Variation;
typedef std::vector<Variation> VariationCollection;

#endif
