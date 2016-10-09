#include "mentry.h"
#include <stdio.h>

int main()
{
	MEntry *me;

	while((me = me_get(stdin)) != NULL)
	{
		me_print(me, stdout);
		me_destroy(me);
	}
	return 0;
}
