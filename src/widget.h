#ifndef BYTE_WIDGET_H
#define BYTE_WIDGET_H

#include "handlemanager.h"
#include "vector.h"
#include "matrix.h"
#include "text.h"

typedef enum {
    WT_Button,
    WT_Window,
    WT_Text
}   WidgetType;

///< The widget structure is basically the mix between an Entity and a Text. It has bounds
///< so we can track mouse interaction.
typedef struct {
    HandleManager *mUsed;

    u32             *mTextureMeshes;       // Mesh for the texture.
    u32             *mTextMeshes;           //  Mesh for the text.
    u32             *mTextures;    //  Texture.
    u32             mDepth;      //  There is one depth for every widget.
    mat3           **mMatrices;   //  For each widget there is a pointer to a matrix.
    vec2            *mBounds;      //  Width and Height of each widget, defines their bounding box.

    const Font      **mFonts;   //  For each widget there is a pointer to a Font.
    Color           *mColors;
    vec2            *mTextPositions;

    char            **mStrings;


    WidgetType *mWidgetTypes;

    u32 mCount,
          mSize,
          mMaxIndex;
} WidgetArray;


typedef enum {
    WA_Texture,
    WA_Matrix,
    WA_Bounds,
    WA_Font,
    WA_Color,
    WA_TextOffset,
    WA_String,
    WA_Type
} WidgetAttrib;

WidgetArray* WidgetArray_init(u32 pSize);

int WidgetArray_add(WidgetArray* pWA);

void WidgetArray_remove(WidgetArray* pWA, u32 pIndex);

void WidgetArray_clear(WidgetArray* pWA);

void WidgetArray_destroy(WidgetArray* pWA);

#endif // BYTE_WIDGET
