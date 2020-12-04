#pragma once
bool Dimmer_Init();
void Dimmer_Show(bool state);
void Dimmer_SetAlpha(float alpha);
void Dimmer_SetColor(float r, float g, float b);
void Dimmer_SetColor(COLORREF rgb);
bool Dimmer_IsDimmerWindow(HWND hwnd);
void Dimmer_Close();