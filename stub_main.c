#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "main.h"

int main(int argc, char* argv[]) {
  edcl_main();

  // Parse arguments:
    if (argc > 1) {
      int i, entry = 0;
        for (i = 1; i < argc; i++) {
	  if (strcmp(argv[i], "-loadelf") == 0) entry=edcl_loadelf(argv[++i]);
	  else if (strcmp(argv[i], "-bootstrap") == 0) edcl_bootstrap(entry);
	  else if (strcmp(argv[i], "-led") == 0) write_led(atoi(argv[++i]));
	  else fprintf(stderr, "Invalid flag %s\n", argv[i]);
        }
    }

    edcl_close();
    return 0;
}
