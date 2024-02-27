#include <Windows.h>
#include "EMI/EMI.h"
#include <iostream>

struct Pixel
{
	short x;
	short y;
	short type;
	short color;

	Pixel()
	{

	}

	Pixel(int x, int y, int c, int co) : x(x), y(y), type(c), color(co) {}
};

constexpr int size_x = 100;
constexpr int size_y = 100;


int win_x = 100;
int win_y = 100;

int BufferSize = size_x * size_y * 6;

char* FinalBuffer = nullptr;
char* OverlayBuffer = nullptr;

int NetworkSize = 0;
Pixel* NetworkBuffer = nullptr;

const unsigned char wall = 219;
const unsigned char wall2 = 178;
const unsigned char wall3 = 177;
const unsigned char wall4 = 176;
const unsigned char shade = 111;
const unsigned char empty = 255;

constexpr const char* RED = "\x1B[31m";
constexpr const char* GRN = "\x1B[32m";
constexpr const char* YEL = "\x1B[33m";
constexpr const char* BLU = "\x1B[34m";
constexpr const char* MAG = "\x1B[35m";
constexpr const char* CYN = "\x1B[36m";
constexpr const char* WHT = "\x1B[37m";

const char* get_color(int c)
{
	switch (c)
	{
	case 0: return WHT;
	case 1: return RED;
	case 2: return GRN;
	case 3: return YEL;
	case 4: return BLU;
	case 5: return MAG;
	case 6: return CYN;
	default:
		return WHT;
	}
}

HANDLE hOut;
HANDLE hIn;

int setup_console()
{
	HWND consoleWindow = GetConsoleWindow();
	SetWindowLong(consoleWindow, GWL_STYLE, GetWindowLong(consoleWindow, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);

	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hIn = GetStdHandle(STD_INPUT_HANDLE);

	CONSOLE_FONT_INFOEX fontInfo;
	fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
	GetCurrentConsoleFontEx(hOut, false, &fontInfo);
	COORD font = { 8, 8 };
	fontInfo.dwFontSize = font;
	fontInfo.FontFamily = FF_ROMAN;
	fontInfo.FontWeight = FW_BOLD;
	wcscpy_s(fontInfo.FaceName, L"Terminal");//SimSun-ExtB
	SetCurrentConsoleFontEx(hOut, false, &fontInfo);

	CONSOLE_SCREEN_BUFFER_INFOEX SBInfo;

	SBInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	GetConsoleScreenBufferInfoEx(hOut, &SBInfo);
	SBInfo.dwSize.X = size_x;
	SBInfo.dwSize.Y = size_y;
	SBInfo.srWindow.Bottom = size_y;
	SBInfo.srWindow.Right = size_x - 1;
	SBInfo.srWindow.Top = 0;
	SBInfo.srWindow.Left = 0;

	SetConsoleScreenBufferInfoEx(hOut, &SBInfo);

	CONSOLE_CURSOR_INFO     cursorInfo;

	GetConsoleCursorInfo(hOut, &cursorInfo);
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(hOut, &cursorInfo);

	DWORD prev_mode;
	GetConsoleMode(hIn, &prev_mode);
	SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS |
		(prev_mode & ~(ENABLE_QUICK_EDIT_MODE | ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_PROCESSED_INPUT)) | ENABLE_MOUSE_INPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	GetConsoleMode(hOut, &prev_mode);
	SetConsoleMode(hOut, prev_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);

	GetConsoleScreenBufferInfoEx(hOut, &SBInfo);

	win_x = SBInfo.dwSize.X;
	win_y = SBInfo.dwSize.Y;

	BufferSize = win_x * win_y * 6;

	FinalBuffer = new char[BufferSize + 1]();
	OverlayBuffer = new char[BufferSize + 1]();
	NetworkBuffer = new Pixel[win_x * win_y]();
	memset(FinalBuffer, 0, BufferSize + 1);
	for (int i = 0; i < BufferSize; i += 6) {
		strncpy_s(&FinalBuffer[i], 6, WHT, 6);
		FinalBuffer[i + 5] = empty;
	}
	memset(OverlayBuffer, 0, BufferSize + 1);

	return 0;
}

void write_pixel(int x, int y, unsigned char c, int color = 0)
{
	int index = (size_x * y + x) * 6;
	if (index >= 0 && index < BufferSize) {
		strncpy_s(&OverlayBuffer[index], 6, get_color(color), 6);
		OverlayBuffer[index + 5] = c;
	}
}

void write_text(int x, int y, const char* text, int color = 0)
{
	int dx = x;
	int dy = y;
	const char* ptr = text;
	while (*ptr != '\0') {
		write_pixel(dx, dy, *ptr, color);
		ptr++;
		dx++;
	}

}

void write_text_centered(int x, int y, const char* text, int color = 0)
{
	int dx = x - strlen(text) / 2;
	int dy = y;
	const char* ptr = text;
	while (*ptr != '\0') {
		write_pixel(dx, dy, *ptr, color);
		ptr++;
		dx++;
	}

}

void render_frame()
{
	SetConsoleCursorPosition(hOut, COORD());
	for (int i = 0; i < BufferSize; i += 6)
	{
		if (OverlayBuffer[i])
			for (int f = 0; f < 6; f++)
				FinalBuffer[i + f] = OverlayBuffer[i + f];
		else
			FinalBuffer[i + 5] = empty;
	}
	std::cout.write(FinalBuffer, BufferSize - 1);
}

int random_in_range(int low, int high) {
	return rand() % (high - low) + low;
}

EMI_REGISTER(Global, setupconsole, setup_console);
EMI_REGISTER(Global, writepixel, write_pixel);
EMI_REGISTER(Global, writetext, write_text);
EMI_REGISTER(Global, writetextCentered, write_text_centered);
EMI_REGISTER(Global, render, render_frame);
EMI_REGISTER(Global, random, random_in_range);