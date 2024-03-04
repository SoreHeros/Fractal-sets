#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>
#include <complex.h>

#define ZOOM 0.1
#define INITIAL_HEIGTH 2.0

struct{
    int width;
    int height;
    int *pixels;
}frame = {0};

int quit = 0;

char keyboard[256] = {0};

struct{
    int x, y;
    int buttons;
}mouse = {0};

enum { MOUSE_LEFT = 0b1, MOUSE_MIDDLE = 0b10, MOUSE_RIGHT = 0b100, MOUSE_X1 = 0b1000, MOUSE_X2 = 0b10000, MOUSE_SCROLL_UP = 0b100000, MOUSE_SCROLL_DOWN = 0b1000000};

static BITMAPINFO frame_bitmap_info;
static HBITMAP frame_bitmap = 0;
static HDC frame_device_context = 0;
static HINSTANCE hInstance;
static int nCmdShow;

LRESULT CALLBACK WindowProcessMessage(HWND, UINT, WPARAM, LPARAM);

void mostrar_pantalla(HWND window_handle){
    InvalidateRect(window_handle, NULL, FALSE);
    UpdateWindow(window_handle);
}

void rellenar_pixels(double B, double L, double T, double R){
    //mostrar cuadrado rojo
    int x, y;
    double delta_x = R - L, delta_y = T - B;
    double real_x, real_y;
    double _Complex C;

    for(y = 0; y < frame.height; y++)
        for(x = 0; x < frame.width; x++) {
            real_x = L + delta_x * (x / (double) frame.width);
            real_y = B + delta_y * (y / (double) frame.height);

            C = real_x + real_y * I;

            //C *= C;

            if (cabs(C) < 1)
                frame.pixels[y * frame.width + x] = 0xff0000;
            else {
                frame.pixels[y * frame.width + x] = 0x000000;
            }
        }
}

void WINAPI proceso_pantalla(){

    WNDCLASS window_class = {0};
    const char window_class_name[] = "My Window Class";
    window_class.lpszClassName = window_class_name;
    window_class.lpfnWndProc = WindowProcessMessage;
    window_class.hInstance = hInstance;

    RegisterClass(&window_class);

    frame_bitmap_info.bmiHeader.biSize = sizeof(frame_bitmap_info.bmiHeader);
    frame_bitmap_info.bmiHeader.biPlanes = 1;
    frame_bitmap_info.bmiHeader.biBitCount = 32;
    frame_bitmap_info.bmiHeader.biCompression = BI_RGB;
    frame_device_context = CreateCompatibleDC(0);

    RECT desktop_rect;
    HWND desktop_handle = GetDesktopWindow();
    if(desktop_handle) GetWindowRect(desktop_handle, &desktop_rect);
    else { desktop_rect.left = 0; desktop_rect.top = 0; desktop_rect.right = 800; desktop_rect.bottom = 600; }
    HWND window_handle = CreateWindow((PCSTR)window_class_name, "pantalla", WS_POPUP, desktop_rect.left,desktop_rect.top, desktop_rect.right - desktop_rect.left,desktop_rect.bottom - desktop_rect.top, NULL, NULL, hInstance, NULL);

    MSG message;
    ShowWindow(window_handle, nCmdShow);

    double L = -(frame.width / (double) frame.height) * INITIAL_HEIGTH / 2, B = -INITIAL_HEIGTH / 2, T = INITIAL_HEIGTH / 2, R = (frame.width / (double) frame.height) * INITIAL_HEIGTH / 2;

    int prev_x = 0, prev_y = 0, delta_x = 0, delta_y = 0;

    double distance_x, distance_y;

    while(!quit){//repetir mientras no se sale

        //procesar mensajes
        mouse.buttons &= ~(MOUSE_SCROLL_DOWN | MOUSE_SCROLL_UP);//limpiar el scroll del ratón
        while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        //get la distancia movida por el ratón
        if(mouse.buttons & (MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE)) {
            delta_x = mouse.x - prev_x;
            delta_y = mouse.y - prev_y;
            prev_x = mouse.x;
            prev_y = mouse.y;
        }else{
            prev_x = mouse.x;
            prev_y = mouse.y;
            delta_x = 0;
            delta_y = 0;
        }

        //mover distancias si se ha movido el ratón
        //sacar la distancia real entre los lados
        distance_x = R - L;
        distance_y = T - B;
        L -= distance_x * (delta_x / (double)frame.width );
        B -= distance_y * (delta_y / (double)frame.height);
        R -= distance_x * (delta_x / (double)frame.width );
        T -= distance_y * (delta_y / (double)frame.height);

        //hacer zoom o no
        if(mouse.buttons & (MOUSE_SCROLL_DOWN)){
            //quitar zoom

            B -= (distance_y * (mouse.y / (double)frame.height)) * ZOOM;
            L -= (distance_x * (mouse.x / (double)frame.width )) * ZOOM;
            T += (distance_y * (1 - mouse.y / (double)frame.height)) * ZOOM;
            R += (distance_x * (1 - mouse.x / (double)frame.width )) * ZOOM;
        }

        if(mouse.buttons & (MOUSE_SCROLL_UP)){
            //aumentar zoom

            B += (distance_y * (mouse.y / (double)frame.height)) * ZOOM;
            L += (distance_x * (mouse.x / (double)frame.width )) * ZOOM;
            T -= (distance_y * (1 - mouse.y / (double)frame.height)) * ZOOM;
            R -= (distance_x * (1 - mouse.x / (double)frame.width )) * ZOOM;
        }

        //rellenar pixeles
        rellenar_pixels(B, L, T, R);

        //mostrar pixels por pantalla
        mostrar_pantalla(window_handle);
    }
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hPrevInstance, PSTR pCmdLine, int numComnd) {

    hInstance = hinst;
    nCmdShow = numComnd;

    proceso_pantalla();

    //liberar memoria
    free(frame.pixels);

    return 0;
}

LRESULT CALLBACK WindowProcessMessage(HWND window_handle, UINT message, WPARAM wParam, LPARAM lParam) {
    static int has_focus = 1;

    switch(message) {

        case WM_PAINT: {
            static PAINTSTRUCT paint;
            static HDC device_context;
            device_context = BeginPaint(window_handle, &paint);
            BitBlt(device_context,
                   paint.rcPaint.left, paint.rcPaint.top,
                   paint.rcPaint.right - paint.rcPaint.left, paint.rcPaint.bottom - paint.rcPaint.top,
                   frame_device_context,
                   paint.rcPaint.left, paint.rcPaint.top,
                   SRCCOPY);
            EndPaint(window_handle, &paint);
        } break;

        case WM_SIZE: {
            frame_bitmap_info.bmiHeader.biWidth  = LOWORD(lParam);
            frame_bitmap_info.bmiHeader.biHeight = HIWORD(lParam);

            if(frame_bitmap) DeleteObject(frame_bitmap);
            frame_bitmap = CreateDIBSection(NULL, &frame_bitmap_info, DIB_RGB_COLORS, (void**)&frame.pixels, 0, 0);
            SelectObject(frame_device_context, frame_bitmap);

            frame.width =  LOWORD(lParam);
            frame.height = HIWORD(lParam);
        } break;



        case WM_KILLFOCUS: {
            has_focus = 0;
            memset(keyboard, 0, 256 * sizeof(keyboard[0]));
            mouse.buttons = 0;
        } break;

        case WM_SETFOCUS: has_focus = 1; break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            if(has_focus) {
                static char key_is_down, key_was_down;
                key_is_down  = (char)((lParam & (1 << 31)) == 0);
                key_was_down = (char)((lParam & (1 << 30)) != 0);
                if(key_is_down != key_was_down) {
                    keyboard[(int)wParam] = key_is_down;
                    if(key_is_down) {
                        switch(wParam) {
                            case VK_ESCAPE: quit = 1;
                            break;
                        }
                    }
                }
            }
        } break;

        case WM_MOUSEMOVE: {
            mouse.x = LOWORD(lParam);
            mouse.y = frame.height - 1 - HIWORD(lParam);
        } break;

        case WM_LBUTTONDOWN: mouse.buttons |=  MOUSE_LEFT;   break;
        case WM_LBUTTONUP:   mouse.buttons &= ~MOUSE_LEFT;   break;
        case WM_MBUTTONDOWN: mouse.buttons |=  MOUSE_MIDDLE; break;
        case WM_MBUTTONUP:   mouse.buttons &= ~MOUSE_MIDDLE; break;
        case WM_RBUTTONDOWN: mouse.buttons |=  MOUSE_RIGHT;  break;
        case WM_RBUTTONUP:   mouse.buttons &= ~MOUSE_RIGHT;  break;

        case WM_XBUTTONDOWN: {
            if(GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
                mouse.buttons |= MOUSE_X1;
            } else { mouse.buttons |= MOUSE_X2; }
        } break;
        case WM_XBUTTONUP: {
            if(GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
                mouse.buttons &= ~MOUSE_X1;
            } else { mouse.buttons &= ~MOUSE_X2; }
        } break;

        case WM_MOUSEWHEEL: {
            if(wParam & 0b10000000000000000000000000000000){
                //down
                mouse.buttons |= MOUSE_SCROLL_DOWN;
                mouse.buttons &= ~MOUSE_SCROLL_UP;
            }else{
                //up
                mouse.buttons |= MOUSE_SCROLL_UP;
                mouse.buttons &= ~MOUSE_SCROLL_DOWN;
            }
        } break;

        default: return DefWindowProc(window_handle, message, wParam, lParam);
    }

    return 0;
}