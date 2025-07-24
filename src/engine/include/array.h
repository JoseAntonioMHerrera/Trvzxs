#pragma once

typedef struct ArrayUint32_t {
    uint32_t* items;
    uint32_t itemsLen;
} ArrayUint32_t;

typedef struct MatrixUint32_t {
    uint32_t **ppItem;
    uint32_t *pItemLen;
    uint32_t itemsLen;
    uint32_t totalItemsLen;

    ArrayUint32_t rowOffset;

} MatrixUint32_t;

void freeArrayUint32_t(ArrayUint32_t* arr);
void freeMatrixUint32_t(MatrixUint32_t* arr);
