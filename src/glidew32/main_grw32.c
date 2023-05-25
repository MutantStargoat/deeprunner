/*
Deep Runner - 6dof shooter game for the SGI O2.
Copyright (C) 2023  John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <windows.h>
#include "glide.h"
#include "game.h"
#include "gaw/gaw_glide.h"
#include "options.h"

static LRESULT CALLBACK handle_message(HWND win, unsigned int msg, WPARAM wparam, LPARAM lparam);
static void handle_mbutton(int bn, int st, WPARAM wparam, LPARAM lparam);
static void warp_mouse(int x, int y);

static HINSTANCE hinst;
static HWND win;
static HDC dc;

static int mapped, upd_pending, quit;
static int warping;
static unsigned long start_time;


int main(int argc, char **argv)
{
	int xsz, ysz;
	MSG msg;
	WNDCLASSEX wc = {0};

	hinst = GetModuleHandle(0);

	wc.cbSize = sizeof wc;
	wc.hbrBackground = GetStockObject(BLACK_BRUSH);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hIcon = wc.hIconSm = LoadIcon(0, IDI_APPLICATION);
	wc.hInstance = hinst;
	wc.lpfnWndProc = handle_message;
	wc.lpszClassName = "grwin32";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	if(!RegisterClassEx(&wc)) {
		fprintf(stderr, "Failed to register window class\n");
		return 1;
	}

	xsz = GetSystemMetrics(SM_CXSCREEN);
	ysz = GetSystemMetrics(SM_CYSCREEN);

	if(!(win = CreateWindow("grwin32", "Deeprunner", WS_POPUP, 0, 0,
					xsz, ysz, 0, 0, hinst, 0))) {
		fprintf(stderr, "Failed to create window\n");
		UnregisterClass("grwin32", hinst);
		return 1;
	}
	dc = GetDC(win);

	ShowWindow(win, 1);
	SetForegroundWindow(win);
	SetFocus(win);
	SetCapture(win);

	start_time = timeGetTime();
	fullscr = 1;
	mouse_grabbed = 1;
	warp_mouse(320, 240);

	gaw_glide_init(640, 480);

	game_reshape(640, 480);

	if(game_init() == -1) {
		return 1;
	}

	while(!quit) {
		while(PeekMessage(&msg, win, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if(quit) break;

		game_display();
	}

	ReleaseCapture();
	game_shutdown();
	return 0;
}

long game_getmsec(void)
{
	return (long)(timeGetTime() - start_time);
}

void game_swap_buffers(void)
{
	grBufferSwap(opt.vsync ? 1 : 0);
}

void game_quit(void)
{
	quit = 1;
}

void game_resize(int x, int y)
{
}

void game_fullscreen(int fs)
{
}

void game_grabmouse(int grab)
{
	static int prev_x, prev_y;

	if(grab == -1) {
		grab = !mouse_grabbed;
	}

	if(grab == mouse_grabbed) return;

	if(grab) {
		warping = 1;
		prev_x = mouse_x;
		prev_y = mouse_y;
		warp_mouse(win_width / 2, win_height / 2);
	} else {
		warping = 1;
		warp_mouse(prev_x, prev_y);
	}
	mouse_grabbed = grab;
}

void game_vsync(int vsync)
{
}

static LRESULT CALLBACK handle_message(HWND win, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	int x, y, key;

	switch(msg) {
	case WM_CLOSE:
		DestroyWindow(win);
		break;

	case WM_DESTROY:
		quit = 1;
		PostQuitMessage(0);
		break;

	case WM_PAINT:
		upd_pending = 1;
		ValidateRect(win, 0);
		break;

	case WM_SIZE:
		x = lparam & 0xffff;
		y = lparam >> 16;
		if(x != win_width && y != win_height) {
			win_width = x;
			win_height = y;
			game_reshape(x, y);
		}
		break;

	case WM_SHOWWINDOW:
		mapped = wparam;
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		key = translate_vkey(wparam);
		game_keyboard(key, 1);
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		key = translate_vkey(wparam);
		game_keyboard(key, 0);
		break;

	case WM_LBUTTONDOWN:
		handle_mbutton(0, 1, wparam, lparam);
		break;
	case WM_MBUTTONDOWN:
		handle_mbutton(1, 1, wparam, lparam);
		break;
	case WM_RBUTTONDOWN:
		handle_mbutton(2, 1, wparam, lparam);
		break;
	case WM_LBUTTONUP:
		handle_mbutton(0, 0, wparam, lparam);
		break;
	case WM_MBUTTONUP:
		handle_mbutton(1, 0, wparam, lparam);
		break;
	case WM_RBUTTONUP:
		handle_mbutton(2, 0, wparam, lparam);
		break;

	case WM_MOUSEMOVE:
		x = lparam & 0xffff;
		y = lparam >> 16;

		if(mouse_grabbed) {
			if(!warping) {
				game_motion(x, y);
				warping = 1;
				warp_mouse(win_width / 2, win_height / 2);
			} else {
				warping = 0;
				mouse_x = x;
				mouse_y = y;
			}
		} else {
			game_motion(x, y);
		}
		break;

	case WM_SYSCOMMAND:
		wparam &= 0xfff0;
		if(wparam == SC_KEYMENU || wparam == SC_SCREENSAVE || wparam == SC_MONITORPOWER) {
			return 0;
		}
	default:
		return DefWindowProc(win, msg, wparam, lparam);
	}

	return 0;
}

static void update_modkeys(void)
{
	if(GetKeyState(VK_SHIFT) & 0x8000) {
		modkeys |= GKEY_MOD_SHIFT;
	} else {
		modkeys &= ~GKEY_MOD_SHIFT;
	}
	if(GetKeyState(VK_CONTROL) & 0x8000) {
		modkeys |= GKEY_MOD_CTRL;
	} else {
		modkeys &= ~GKEY_MOD_CTRL;
	}
	if(GetKeyState(VK_MENU) & 0x8000) {
		modkeys |= GKEY_MOD_ALT;
	} else {
		modkeys &= ~GKEY_MOD_ALT;
	}
}

#ifndef VK_OEM_1
#define VK_OEM_1	0xba
#define VK_OEM_2	0xbf
#define VK_OEM_3	0xc0
#define VK_OEM_4	0xdb
#define VK_OEM_5	0xdc
#define VK_OEM_6	0xdd
#define VK_OEM_7	0xde
#endif

static int translate_vkey(int vkey)
{
	switch(vkey) {
	case VK_PRIOR: return GKEY_PGUP;
	case VK_NEXT: return GKEY_PGDOWN;
	case VK_END: return GKEY_END;
	case VK_HOME: return GKEY_HOME;
	case VK_LEFT: return GKEY_LEFT;
	case VK_UP: return GKEY_UP;
	case VK_RIGHT: return GKEY_RIGHT;
	case VK_DOWN: return GKEY_DOWN;
	case VK_OEM_1: return ';';
	case VK_OEM_2: return '/';
	case VK_OEM_3: return '`';
	case VK_OEM_4: return '[';
	case VK_OEM_5: return '\\';
	case VK_OEM_6: return ']';
	case VK_OEM_7: return '\'';
	default:
		break;
	}

	if(vkey >= 'A' && vkey <= 'Z') {
		vkey += 32;
	} else if(vkey >= VK_F1 && vkey <= VK_F12) {
		vkey -= VK_F1 + GKEY_F1;
	}

	return vkey;
}

static void handle_mbutton(int bn, int st, WPARAM wparam, LPARAM lparam)
{
	int x, y;

	update_modkeys();

	x = lparam & 0xffff;
	y = lparam >> 16;
	game_mouse(bn, st, x, y);
}

static void warp_mouse(int x, int y)
{
	/*POINT pt;
	pt.x = x;
	pt.y = y;

	ClientToScreen(win, &pt);
	SetCursorPos(pt.x, pt.y);*/
	SetCursorPos(x, y);
}
