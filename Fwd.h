// $Id$
// Copyright Daniel Lidstrom
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.
//! @file
//! Brief.
//! @author Daniel Lidstrom <dlidstrom@gmail.com>
//! @date   2009-11-30 17:25
//! @ingroup
//!

#if !defined(FWD_H__20091130T1725)
#define FWD_H__20091130T1725

#include <memory>                                 // shared_ptr
#include <vector>

class CBook;
typedef std::shared_ptr<CBook> CBookPtr;
class CBookData;
class CBoni;
class CCache;
class CCalcParams;
typedef std::shared_ptr<CCalcParams> CCalcParamsPtr;
class CCalcParamsFixedHeight;
typedef std::shared_ptr<CCalcParamsFixedHeight> CCalcParamsFixedHeightPtr;
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
typedef std::shared_ptr<CPlayer> CPlayerPtr;
class CPlayerComputer;
typedef std::shared_ptr<CPlayerComputer> CPlayerComputerPtr;
class CQPosition;
class CSavedGame;
class CSearchInfo;
struct CWindow;
struct Variation;
typedef std::vector<Variation> VariationCollection;

#endif
