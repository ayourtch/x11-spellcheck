#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A little experiment with auto-spellchecking in X11 */
/* (c) Andrew Yourtchenko ayourtch@gmail.com, MIT license */

typedef struct tuple_t {
  char *search;
  char *replace;
} tuple_t;

tuple_t xlate[] = {
  
  { "int tnu", "\b\b\btun" },

  { NULL, 0 },
};

  char txtbuf[16] = { };
  int txtidx = sizeof(txtbuf)-1; // Leave space for the string terminator

/* Send Fake Key Event */
static void SendKey (Display * disp, KeySym keysym, KeySym modsym)
{
  KeyCode keycode = 0, modcode = 0;

  keycode = XKeysymToKeycode (disp, keysym);
  if (keycode == 0) return;

  XTestGrabControl (disp, True);

  /* Generate modkey press */
  if (modsym != 0)
  {
     modcode = XKeysymToKeycode(disp, modsym);
     XTestFakeKeyEvent (disp, modcode, True, 0);
  }

  /* Generate regular key press and release */
  XTestFakeKeyEvent (disp, keycode, True, 0);
  XTestFakeKeyEvent (disp, keycode, False, 0);

  /* Generate modkey release */
  if (modsym != 0)
    XTestFakeKeyEvent (disp, modcode, False, 0);

  XSync (disp, False);
  XTestGrabControl (disp, False);
}

void InjectKey(Display *d, KeySym key) {
  XUngrabKeyboard(d, CurrentTime);
  SendKey (d, key, 0);
  XGrabKeyboard(d, DefaultRootWindow(d), True, GrabModeAsync, GrabModeAsync, CurrentTime);
}

void TrySpellcheck(Display *d, KeySym key) {
  int i = 0;
  char *match;
  char *buf;
  printf("TrySpellCheck\n");
  XUngrabKeyboard(d, CurrentTime);
  SendKey (d, key, 0);
  while(xlate[i].search) {
    printf("Matching: '%s', '%s'\n", &txtbuf[txtidx], xlate[i].search);
    match = xlate[i].search + strlen(xlate[i].search);
    buf = &txtbuf[txtidx] + strlen(&txtbuf[txtidx]);
    while((buf >= &txtbuf[txtidx]) && (match >= xlate[i].search) && (*buf == *match)) {
      buf--; match--;
    }
    if(match < xlate[i].search) {
      int k = 0;
      printf("Matched %s!\n", xlate[i].search);
      while(xlate[i].replace[k]) {  
        if (xlate[i].replace[k] == '\b') {
          SendKey (d, XK_BackSpace, 0);
        } else {
          SendKey (d, xlate[i].replace[k], 0);
        }
        k++;
      }
    }
    i++;
  }
  XGrabKeyboard(d, DefaultRootWindow(d), True, GrabModeAsync, GrabModeAsync, CurrentTime);
}
 
int main(void) {
  Display *d;
  Window w;
  XEvent e;
  KeySym key;		

  int s;

  d = XOpenDisplay(NULL);
  if (d == NULL) {
    fprintf(stderr, "Cannot open display\n");
    exit(1);
  }

  s = DefaultScreen(d);

  w = XCreateSimpleWindow(d, RootWindow(d, s), 10, 10, 200, 200, 1,
			  BlackPixel(d, s), WhitePixel(d, s));

  XSelectInput(d, w, ExposureMask | KeyPressMask | KeyReleaseMask);

  XMapWindow(d, w);

  XGrabKeyboard(d, DefaultRootWindow(d), True, GrabModeAsync, GrabModeAsync, CurrentTime);
  while (1) {
    XNextEvent(d, &e);
    if (e.type == KeyPress) {
      key = XKeycodeToKeysym(d, e.xkey.keycode, 0);
        
    } else if (e.type == KeyRelease) { 
      key = XKeycodeToKeysym(d, e.xkey.keycode, 0);
      printf("Release: %c\n", key);
      // This is really a hack since it does not correctly handle non-alnum keys.
      if(txtidx > 0) {
        txtidx--;
      }
      memmove(txtbuf, txtbuf+1, sizeof(txtbuf)-2); 
      txtbuf[sizeof(txtbuf)-2] = (char)key;
      TrySpellcheck(d, key);
    }
    if (e.type == KeyPress) { 
      key = XKeycodeToKeysym(d, e.xkey.keycode, 0);
      if (key == XK_Alt_L) {
        // Exit on pressing Alt
	break;
      }
    }
  }
  XUngrabKeyboard(d, CurrentTime);

  XCloseDisplay(d);

  return 0;
}

