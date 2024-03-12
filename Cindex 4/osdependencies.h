typedef int time_c;

typedef struct {
	float x;
	float y;
} IRPoint;

typedef struct {
	float width;		/* should never be negative */
	float height;		/* should never be negative */
} IRSize;

typedef struct {
	IRPoint origin;
	IRSize size;
} IRRect;

#if defined CIN_MAC_OS

#define V2_CHARSET "MacRoman"
#define V2_FOREIGN "CP1252"
#define UNKNOWNCHAR 240		// apple logo

#define DMORIENT_PORTRAIT 1
#define DMORIENT_LANDSCAPE 2
typedef struct {
    int    left;
    int    top;
    int    right;
    int    bottom;
} RECT;

#else	// Windows
#define V2_CHARSET "CP1252"		// check this
#define V2_FOREIGN "MacRoman"
#define UNKNOWNCHAR 191			// inverted question mark

typedef unsigned short unichar;

#if 0
typedef struct {
    float x;
    float y;
} NSPoint;

typedef struct {
    float width;		/* should never be negative */
    float height;		/* should never be negative */
} NSSize;

typedef struct {
    NSPoint origin;
    NSSize size;
} NSRect;
#endif

#endif
