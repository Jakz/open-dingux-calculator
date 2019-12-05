#include "views/view_manager.h"

/*
* D-PAD Left - SDLK_LEFT
* D-PAD Right - SDLK_RIGHT
* D-PAD Up - SDLK_UP
* D-PAD Down - SDLK_DOWN
* Y button - SDLK_SPACE
* X button - SDLK_LSHIFT
* A button - SDLK_LCTRL
* B button - SDLK_LALT
* START button - SDLK_RETURN
* SELECT button - SDLK_ESC
* L shoulder - SDLK_TAB
* R shoulder - SDLK_BACKSPACE
* Power slider in up position - SDLK_POWER (not encouraged to map in game, as it's used by the pwswd daemon)
* Power slider in down position - SDLK_PAUSE

*/

#include <sys/resource.h>

void setStackSize(size_t size)
{
  const rlim_t kStackSize = size;   // min stack size = 16 MB
  struct rlimit rl;
  int result;

  result = getrlimit(RLIMIT_STACK, &rl);
  if (result == 0)
  {
      if (rl.rlim_cur < kStackSize)
      {
          rl.rlim_cur = kStackSize;
          result = setrlimit(RLIMIT_STACK, &rl);
          if (result != 0)
          {
              fprintf(stderr, "setrlimit returned result = %d\n", result);
          }
      }
  }
}

int main(int argc, char* argv[])
{
  setStackSize(16*1024*1024);

  ui::ViewManager ui;

  if (!ui.init())
    return -1;

  if (!ui.loadData())
  {
    printf("Error while loading and initializing data.\n");
    ui.deinit();
    return -1;
  }

  ui.loop();
  ui.deinit();

  return 0;
}
