#ifndef ARCHIVISTA_ZOOM_H
#define ARCHIVISTA_ZOOM_H

#include "main.h"
#include "app.h"

// Zoom constraints
#define ZOOM_DEFAULT  FONT_HEIGHT   // Ukuran font default (sama dengan FONT_HEIGHT)
#define ZOOM_MIN       8   // Ukuran font minimum (jangan terlalu kecil)
#define ZOOM_MAX      72   // Ukuran font maksimum
#define ZOOM_STEP      2   // Berapa pt naik/turun per satu zoom action

// ========== Function Declarations ==========

// Menerapkan fontSize saat ini ke font editor.
// Menghapus font lama, membuat font baru, menghitung ulang charWidth/charHeight, lalu repaint.
void Zoom_Apply(HWND hWnd, AppState *s);

// Zoom In: fontSize += ZOOM_STEP (max ZOOM_MAX), lalu Zoom_Apply.
void Zoom_In(HWND hWnd, AppState *s);

// Zoom Out: fontSize -= ZOOM_STEP (min ZOOM_MIN), lalu Zoom_Apply.
void Zoom_Out(HWND hWnd, AppState *s);

// Reset Zoom: fontSize = ZOOM_DEFAULT, lalu Zoom_Apply.
void Zoom_Reset(HWND hWnd, AppState *s);

#endif //ARCHIVISTA_ZOOM_H
