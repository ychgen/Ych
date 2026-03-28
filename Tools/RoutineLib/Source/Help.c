#include "RoutineLib.h"

#include <stdio.h>

int RoutineLib_Routine_Help(const Routine* pSelf)
{
    size_t szRoutines;
    Routine* pRoutines = GetRegisteredRoutines(&szRoutines);

    printf("List of Available Actions (%lu in total):\n", szRoutines);
    for (size_t i = 0; i < szRoutines; i++)
    {
        Routine* routine = pRoutines + i;
        printf("%s: %s\n  Usage: %s\n", routine->Name, routine->Description, routine->Usage);
    }

    return ROUTINE_OK;
}
