#ifndef GGROUPENUM_H
#define GGROUPENUM_H

#include <cstdint>

enum class g_group_1 : uint8_t
{
    UNDEF = 0,
    G0,
    G1,
    G2,
    G3,
    CIP,
    ASPLINE,
    BSPLINE,
    CSPLINE,
    POLY,
    G33,
    G331,
    G332,
    OEMIPO1,
    OEMIPO2,
    CT,
    G34,
    G35,
    INVCW,
    INVCCW,
    G335,
    G336,
    MAX = G336
};

enum class g_group_2 : uint8_t
{
    UNDEF = 0,
    G4,
    G63,
    G74,
    G75,
    REPOSL,
    REPOSQ,
    REPOSH,
    REPOSA,
    REPOSQA,
    REPOSHA,
    G147,
    G247,
    G347,
    G148,
    G248,
    G348,
    G5,
    G7,
    MAX = G7
};

enum class g_group_3 : uint8_t
{
    UNDEF = 0,
    TRANS,
    ROT,
    SCALE,
    MIRROR,
    ATRANS,
    AROT,
    ASCALE,
    AMIRROR,
    Reserved,
    G25,
    G26,
    G110,
    G111,
    G112,
    G58,
    G59,
    ROTS,
    AROTS,
    MAX = AROTS
};

enum class g_group_4 : uint8_t
{
    UNDEF = 0,
    STARTFIFO,
    STOPFIFO,
    FIFOCTRL,
    MAX = FIFOCTRL
};

enum class g_group_6 : uint8_t
{
    UNDEF = 0,
    G17,
    G18,
    G19,
    MAX = G19
};

enum class g_group_7 : uint8_t
{
    UNDEF = 0,
    G40,
    G41,
    G42,
    MAX = G42
};

enum class g_group_9 : uint8_t
{
    UNDEF = 0,
    G53,
    SUPA,
    G153,
    MAX = G153
};

enum class g_group_10 : uint8_t
{
    UNDEF = 0,
    G60,
    G64,
    G641,
    G642,
    G643,
    G644,
    G645,
    MAX = G645
};

enum class g_group_11 : uint8_t
{
    UNDEF = 0,
    G9,
    MAX = G9
};

enum class g_group_12 : uint8_t
{
    UNDEF = 0,
    G601,
    G602,
    G603,
    MAX = G603
};

enum class g_group_13 : uint8_t
{
    UNDEF = 0,
    G70,
    G71,
    G700,
    G710,
    MAX = G710
};

enum class g_group_14 : uint8_t
{
    UNDEF = 0,
    G90,
    G91,
    MAX = G91
};

enum class g_group_15 : uint8_t
{
    UNDEF = 0,
    G93,
    G94,
    G95,
    G96,
    G97,
    G931,
    G961,
    G971,
    G942,
    G952,
    G962,
    G972,
    G973,
    MAX = G973
};

enum class g_group_16 : uint8_t
{
    UNDEF = 0,
    CFC,
    CFTCP,
    CFIN,
    MAX = CFIN
};

enum class g_group_17 : uint8_t
{
    UNDEF = 0,
    NORM,
    KONT,
    KONTT,
    KONTC,
    MAX = KONTC
};

enum class g_group_18 : uint8_t
{
    UNDEF = 0,
    G450,
    G451,
    MAX = G451
};

enum class g_group_19 : uint8_t
{
    UNDEF = 0,
    BNAT,
    BTAN,
    BAUTO,
    MAX = BAUTO
};

enum class g_group_20 : uint8_t
{
    UNDEF = 0,
    ENAT,
    ETAN,
    EAUTO,
    MAX = EAUTO
};

enum class g_group_21 : uint8_t
{
    UNDEF = 0,
    BRISK,
    SOFT,
    DRIVE,
    MAX = DRIVE
};

enum class g_group_22 : uint8_t
{
    UNDEF = 0,
    CUT2D,
    CUT2DF,
    CUT3DC,
    CUT3DF,
    CUT3DFS,
    CUT3DFF,
    CUT3DCC,
    CUT3DCCD,
    CUT2DD,
    CUT2DFD,
    CUT3DCD,
    MAX = CUT3DCD
};

enum class g_group_23 : uint8_t
{
    UNDEF = 0,
    CDOF,
    CDON,
    CDOF2,
    MAX = CDOF2
};

enum class g_group_24 : uint8_t
{
    UNDEF = 0,
    FFWOF,
    FFWON,
    MAX = FFWON
};

enum class g_group_25 : uint8_t
{
    UNDEF = 0,
    ORIWKS,
    ORIMKS,
    MAX = ORIMKS
};

enum class g_group_26 : uint8_t
{
    UNDEF = 0,
    RMB,
    RMI,
    RME,
    RMN,
    MAX = RMN
};

enum class g_group_27 : uint8_t
{
    UNDEF = 0,
    ORIC,
    ORID,
    MAX = ORID
};

enum class g_group_28 : uint8_t
{
    UNDEF = 0,
    WALIMON,
    WALIMOF,
    MAX = WALIMOF
};

enum class g_group_29 : uint8_t
{
    UNDEF = 0,
    DIAMOF,
    DIAMON,
    DIAM90,
    DIAMCYCOF,
    MAX = DIAMCYCOF
};

enum class g_group_30 : uint8_t
{
    UNDEF = 0,
    COMPOF,
    COMPON,
    COMPCURV,
    COMPCAD,
    COMPSURF,
    MAX = COMPSURF
};

#endif // GGROUPENUM_H
