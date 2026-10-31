// Decompiled by Opus, deepseek-v4.1-flash, Sonnet 5.5, space-bunny-free and Haiku. Names are provisional.
// The software mouse cursor: the hide/show refcount blits, the mouse-event
// ring ("MOUSE EVENTS"), the saved-background surfaces ("SAVEMOUSE 1" to 3),
// the cursor blit thread and the two spin locks it takes.
#include <windows.h>
#include <string.h>

#include "../graphics/gaf_frame.h"
#include "../graphics/surface.h"
#include "mouse_event.h"
#include "../graphics/display_context.h"

DisplayContext* GetDisplay(void);

extern LONG g_gfxBlitLockHeld;
extern LONG g_gfxBlitLockOwner;
extern HANDLE g_gfxBlitLockEvent;

void __cdecl BlitSurface(void* dst, void* src, int x, int y);
void __stdcall DrawFrame(void* dst, GafFrame* bmp, int x, int y);
void __stdcall DrawSurface(Surface* dst, Surface* bmp, int x, int y);
int __stdcall LockPrimary(void* out);
int __stdcall UnlockPrimary(void* unused, RECT* r1, RECT* r2);
void __stdcall ResetClipRect(void* d);
void* __stdcall AllocSurface(char* name, int width, int height);
void __stdcall FreeSurface(void* obj);
int __stdcall StartThread(void* start, unsigned int stack, void* param);
void __stdcall SleepMilliseconds(unsigned int ms);
void* __cdecl GameAllocIgnoreTag(char* name, unsigned int size);
void __cdecl GameFreeThunk(void* p);

void __stdcall RedrawMouseCursor(DisplayContext* app);
void __cdecl MouseThreadProc(int param);

// The copy must stay memcpy: a struct assignment computes the source with
// mov/add, not lea.
static inline void GetRect(MouseEvent* out)
{
    DisplayContext* p = GetDisplay();
    memcpy(out, &p->curMouseEvent, sizeof(MouseEvent));
}

// The cursor thread's lock, held on the "MOUS" tag.
static inline LONG LockMouse()
{
    while (1) {
        LONG r = InterlockedExchange(&g_gfxBlitLockHeld, 0x4d4f5553);
        if (r == 0) {
            g_gfxBlitLockOwner = 0x4d4f5553;
            return 0;
        }
        if (g_gfxBlitLockOwner == 0x4d4f5553)
            return r;
        WaitForSingleObject(g_gfxBlitLockEvent, INFINITE);
    }
}

static inline void UnlockMouse(LONG held)
{
    if (held == 0) {
        g_gfxBlitLockOwner = 0;
        InterlockedExchange(&g_gfxBlitLockHeld, 0);
        SetEvent(g_gfxBlitLockEvent);
    }
}

// The blit lock the frame setter takes, held on the "MAIN" tag.
static inline LONG LockMain()
{
    while (1) {
        LONG r = InterlockedExchange(&g_gfxBlitLockHeld, 0x4d41494e);
        if (r == 0) {
            g_gfxBlitLockOwner = 0x4d41494e;
            return 0;
        }
        if (g_gfxBlitLockOwner == 0x4d41494e)
            return r;
        WaitForSingleObject(g_gfxBlitLockEvent, INFINITE);
    }
}

static inline void UnlockMain(LONG held)
{
    if (held == 0) {
        g_gfxBlitLockOwner = 0;
        InterlockedExchange(&g_gfxBlitLockHeld, 0);
        SetEvent(g_gfxBlitLockEvent);
    }
}

// Blits the cursor's current frame at its position when the cursor is
// visible (cursorHideCount <= 0) and the mouse thread does not own the blit
// (cursorThreadEnabled != 1).
// FUNCTION: 0x4c2380
void DrawSoftwareCursor()
{
    DisplayContext* obj = GetDisplay();
    if (obj->cursorThreadEnabled != 1 && obj->cursorHideCount <= 0) {
        MouseEvent r;
        GetRect(&r);
        DrawFrame(0, obj->cursorGafFrame, r.x, r.y);
    }
}

// Draws the saved bitmap at the object's position: locks the screen surface
// (LockPrimary), blits with the hand-written routine BlitSurface, then
// unlocks with the rectangle that was drawn.
// FUNCTION: 0x4c23e0
void __stdcall RestoreCursorBackground(DisplayContext* obj)
{
    if (obj->cursorOverlayEnabled) {
        Surface* bmp = obj->saveMouse1;
        Surface s;
        if (LockPrimary(&s)) {
            BlitSurface(&s, bmp, obj->cursorBlitX, obj->cursorBlitY);
            RECT r;
            r.left = obj->cursorBlitX;
            r.top = obj->cursorBlitY;
            r.right = r.left + bmp->width;
            r.bottom = r.top + bmp->height;
            UnlockPrimary(&s, &r, 0);
        }
    }
}

// Increments the hide counter and restores the saved background from the
// screen when the counter was zero.
// FUNCTION: 0x4c2470
void HideSoftwareCursor()
{
    DisplayContext* o = GetDisplay();
    if (o->cursorThreadEnabled != 1 && o->cursorHideCount++ == 0)
        DrawSurface(0, o->saveMouse1, o->cursorBlitX, o->cursorBlitY);
}

// Captures the background under the cursor position and blits the cursor's
// bitmap there.
// FUNCTION: 0x4c24b0
void __stdcall CaptureBackgroundAndDrawCursor(DisplayContext* obj)
{
    if (obj->cursorOverlayEnabled) {
        Surface s;
        if (LockPrimary(&s)) {
            RECT r;
            POINT pt;
            GetCursorPos(&pt);
            obj->curMouseEvent.x = pt.x;
            obj->curMouseEvent.y = pt.y;
            obj->cursorBlitX = pt.x - obj->cursorGafFrame->xOffset;
            obj->cursorBlitY = pt.y - obj->cursorGafFrame->yOffset;
            obj->saveMouse1->width = obj->cursorGafFrame->width;
            obj->saveMouse1->height = obj->cursorGafFrame->height;
            obj->saveMouse1->pitch = obj->cursorGafFrame->width;
            BlitSurface(obj->saveMouse1, &s, -obj->cursorBlitX, -obj->cursorBlitY);
            DrawFrame(&s, obj->cursorGafFrame, obj->curMouseEvent.x, obj->curMouseEvent.y);
            r.left = obj->cursorBlitX;
            r.top = obj->cursorBlitY;
            r.right = r.left + obj->cursorGafFrame->width;
            r.bottom = r.top + obj->cursorGafFrame->height;
            UnlockPrimary(&s, &r, 0);
        }
    }
}

// Redraws the software mouse cursor: reads the cursor position, computes the
// cursor's top left (position minus the sprite's origin), rebuilds the saved
// background and the composed image in the three off-screen surface
// descriptors at +0x1be, +0x1c2 and +0x1c6 (each resized to the sprite),
// restores what the old cursor covered, draws the sprite, and hands the union
// of the old and new cursor rectangles to UnlockPrimary to be shown.
// FUNCTION: 0x4c25e0
void __stdcall RedrawMouseCursor(DisplayContext* app)
{
    // info doubles as the first rect for BlitSurface: no separate rect local.
    Surface info;
    RECT r1;
    RECT r2;
    POINT pt;
    if (app->cursorOverlayEnabled == 0)
        return;
    if (LockPrimary(&info) == 0)
        return;
    GetCursorPos(&pt);
    int x = pt.x;
    int y = pt.y;
    app->curMouseEvent.x = x;
    app->curMouseEvent.y = y;
    x -= app->cursorGafFrame->xOffset;
    y -= app->cursorGafFrame->yOffset;
    app->saveMouse2->width = app->cursorGafFrame->width;
    app->saveMouse2->height = app->cursorGafFrame->height;
    app->saveMouse2->pitch = app->cursorGafFrame->width;
    app->saveMouse3->width = app->cursorGafFrame->width;
    app->saveMouse3->height = app->cursorGafFrame->height;
    app->saveMouse3->pitch = app->cursorGafFrame->width;
    BlitSurface(app->saveMouse2, &info, -x, -y);
    BlitSurface(app->saveMouse2, app->saveMouse1, app->cursorBlitX - x, app->cursorBlitY - y);
    BlitSurface(app->saveMouse3, app->saveMouse2, 0, 0);
    ResetClipRect(app->saveMouse3);
    DrawFrame(app->saveMouse3, app->cursorGafFrame, app->cursorGafFrame->xOffset, app->cursorGafFrame->yOffset);
    BlitSurface(app->saveMouse1, app->saveMouse3, x - app->cursorBlitX, y - app->cursorBlitY);
    r1.left = app->cursorBlitX;
    r1.top = app->cursorBlitY;
    // Right and bottom computed from the just-stored left and top fields.
    r1.right = r1.left + app->saveMouse1->width;
    r1.bottom = app->saveMouse1->height + r1.top;
    r2.left = x;
    r2.top = y;
    r2.right = x + app->cursorGafFrame->width;
    r2.bottom = y + app->cursorGafFrame->height;
    BlitSurface(&info, app->saveMouse1, app->cursorBlitX, app->cursorBlitY);
    BlitSurface(&info, app->saveMouse3, x, y);
    app->saveMouse1->width = app->cursorGafFrame->width;
    app->saveMouse1->height = app->cursorGafFrame->height;
    app->saveMouse1->pitch = app->cursorGafFrame->width;
    BlitSurface(app->saveMouse1, app->saveMouse2, 0, 0);
    app->cursorBlitX = x;
    app->cursorBlitY = y;
    UnlockPrimary(&info, &r1, &r2);
}

// Moves the cursor towards the position: decrements the hide counter, reads
// the cursor, stores it as the object's rectangle, offsets the object by the
// bitmap's half size, blits it from the screen with DrawSurface, then, if the
// counter is still not positive, copies the rectangle and draws the bitmap
// with DrawFrame.
// Must be __fastcall: the original file was built with /Gr.
// FUNCTION: 0x4c2870
void __fastcall ShowSoftwareCursor(void)
{
    DisplayContext* o = GetDisplay();
    if (o->cursorThreadEnabled != 1) {
        if (--o->cursorHideCount <= 0) {
            POINT pt;
            GetCursorPos(&pt);
            o->curMouseEvent.x = pt.x;
            o->curMouseEvent.y = pt.y;
            if (o->cursorGafFrame != 0) {
                o->cursorBlitX = pt.x - o->cursorGafFrame->xOffset;
                o->cursorBlitY = pt.y - o->cursorGafFrame->yOffset;
                o->saveMouse1->width = o->cursorGafFrame->width;
                o->saveMouse1->height = o->cursorGafFrame->height;
                o->saveMouse1->pitch = o->cursorGafFrame->width;
            }
            DrawSurface(o->saveMouse1, 0, -o->cursorBlitX, -o->cursorBlitY);
            DisplayContext* o2 = GetDisplay();
            if (o2->cursorThreadEnabled != 1 && o2->cursorHideCount <= 0) {
                MouseEvent r;
                GetRect(&r);
                DrawFrame(0, o2->cursorGafFrame, r.x, r.y);
            }
        }
    }
}

// The worker thread: redraws the cursor every 0x21 ticks until the stop flag
// at +0x1d6 is set, then clears it.
// FUNCTION: 0x4c2990
void __cdecl MouseThreadProc(int param)
{
    DisplayContext* o = (DisplayContext*)param;
    SetThreadPriority(GetCurrentThread(), 2);
    while (o->mouseThreadStopReq == 0) {
        int start = GetTickCount() + 0x21;
        LONG held = LockMouse();
        if (o->cursorGafFrame != 0)
            RedrawMouseCursor(o);
        UnlockMouse(held);
        Sleep(max(1, start - (int)GetTickCount()));
    }
    o->mouseThreadStopReq = 0;
}

// Starts the worker thread (0x4c2990) that StopMouseThread stops.
// FUNCTION: 0x4c2a80
int __stdcall StartMouseThread(DisplayContext* s)
{
    s->mouseThreadStopReq = 0;
    s->mouseThreadActive = StartThread((void*)MouseThreadProc, 0x8000, s);
    if (s->mouseThreadActive) {
        s->cursorThreadEnabled = 1;
        return 1;
    }
    return 0;
}

// Asks the worker to stop and waits up to 20 x 100 ms for it to acknowledge.
// FUNCTION: 0x4c2ac0
int __stdcall StopMouseThread(DisplayContext* s)
{
    if (s->mouseThreadActive == 0)
        return 1;
    s->mouseThreadStopReq = 1;
    int tries = 0;
    while (s->mouseThreadStopReq != 0) {
        if (++tries > 20)
            return 0;
        SleepMilliseconds(100);
    }
    s->mouseThreadActive = 0;
    return 1;
}

// Stores `value` at +0x1b2 of the object GetDisplay returns while holding
// the "MAIN" spin lock; a lock already held by "MAIN" is not released here.
// FUNCTION: 0x4c2b20
void __stdcall SetCursorSprite(int value)
{
    LONG held = LockMain();
    GetDisplay()->cursorGafFrame = (GafFrame*)value;
    UnlockMain(held);
}

// Returns the shared cursor bitmap pointer.
// FUNCTION: 0x4c2ba0
int GetCursorSprite()
{
    return (int)GetDisplay()->cursorGafFrame;
}

// Resets the event ring buffer's head and tail.
// FUNCTION: 0x4c2bb0
void ClearMouseEventQueue()
{
    DisplayContext* p = GetDisplay();
    p->mouseEventWriteIdx = 0;
    p->mouseEventReadIdx = 0;
}

// Initialises the mouse-event input object: allocates the event queue,
// resets it, creates the three save-mouse surfaces, and optionally starts
// the worker thread at 0x4c2990. The second GetDisplay call is ClearMouseEventQueue
// inlined.
// FUNCTION: 0x4c2bd0
void __stdcall InitMouse(int count, int start)
{
    DisplayContext* p = GetDisplay();
    p->mouseEventQueueCap = count;
    p->mouseEventQueue = (MouseEvent*)GameAllocIgnoreTag("MOUSE EVENTS", count * 0x18);
    DisplayContext* q = GetDisplay();
    q->mouseEventWriteIdx = 0;
    q->mouseEventReadIdx = 0;
    p->cursorGafFrame = 0;
    p->saveMouse1 = (Surface*)AllocSurface("SAVEMOUSE 1", 0x640, 1);
    p->saveMouse2 = (Surface*)AllocSurface("SAVEMOUSE 2", 0x640, 1);
    p->saveMouse3 = (Surface*)AllocSurface("SAVEMOUSE 3", 0x640, 1);
    p->cursorHideCount = 1;
    p->cursorOverlayEnabled = 0;
    p->cursorThreadEnabled = 0;
    p->mouseThreadStopReq = 0;
    if (start == 1) {
        p->mouseThreadStopReq = 0;
        p->mouseThreadActive = StartThread((void*)MouseThreadProc, 0x8000, p);
        if (p->mouseThreadActive != 0)
            p->cursorThreadEnabled = 1;
    } else {
        p->mouseThreadActive = 0;
    }
}

// Shuts down the object GetDisplay returns: when flag bit 10 is set and
// the worker at +0x1ca is running, asks it to stop (+0x1d6) and waits up to
// 20 x 100 ms for it to acknowledge; then releases the three objects at
// +0x1be..+0x1c6 and frees the event buffer at +0x18a.
// FUNCTION: 0x4c2cc0
void ShutdownMouse()
{
    DisplayContext* o = GetDisplay();
    if (o->mouseEventQueue) {
        // Separate nested ifs, not `&&`: it changes how the bit is tested.
        if (o->flags.bits.mouseThread) {
            if (o->mouseThreadActive) {
                o->mouseThreadStopReq = 1;
                int i = 0;
                // while (1) keeps the loop test at the top.
                while (1) {
                    if (++i > 20)
                        break;
                    SleepMilliseconds(100);
                    if (!o->mouseThreadStopReq) {
                        o->mouseThreadActive = 0;
                        break;
                    }
                }
            }
        }
        FreeSurface(o->saveMouse3);
        FreeSurface(o->saveMouse2);
        FreeSurface(o->saveMouse1);
        GameFreeThunk(o->mouseEventQueue);
        o->mouseEventQueue = 0;
    }
}

// FUNCTION: 0x4c2d60
int __stdcall PopMouseEvent(MouseEvent* out)
{
    DisplayContext* q = GetDisplay();
    if (q->mouseEventWriteIdx == q->mouseEventReadIdx) {
        *out = q->curMouseEvent;
        return 0;
    }
    *out = q->mouseEventQueue[q->mouseEventReadIdx];
    if (++q->mouseEventReadIdx == q->mouseEventQueueCap) {
        q->mouseEventReadIdx = 0;
    }
    return 1;
}

// Peeks at the next event in the queue without removing it (compare the
// pop at 0x4c2d60).
// FUNCTION: 0x4c2de0
int __stdcall PeekMouseEvent(MouseEvent* out)
{
    DisplayContext* q = GetDisplay();
    if (q->mouseEventWriteIdx == q->mouseEventReadIdx) {
        *out = q->curMouseEvent;
        return 0;
    }
    *out = q->mouseEventQueue[q->mouseEventReadIdx];
    return 1;
}

// Pushes an event onto the queue unless it is full (compare the pop at
// 0x4c2d60).
// FUNCTION: 0x4c2e30
void __stdcall PushMouseEvent(MouseEvent* ev)
{
    DisplayContext* q = GetDisplay();
    if ((q->mouseEventWriteIdx + 1) % q->mouseEventQueueCap != q->mouseEventReadIdx) {
        q->mouseEventQueue[q->mouseEventWriteIdx] = *ev;
        if (++q->mouseEventWriteIdx == q->mouseEventQueueCap) {
            q->mouseEventWriteIdx = 0;
        }
    }
}
