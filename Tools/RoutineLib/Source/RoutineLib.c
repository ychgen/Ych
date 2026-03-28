#include "RoutineLib.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

Routine* g_pRoutines;
size_t   g_szRoutines;

extern int RoutineLib_Routine_Help(const Routine* pSelf);

bool RoutineLibInit(void)
{
    Routine help = MakeRoutine(RoutineLib_Routine_Help, "Help", "Displays all available actions.");
    return RegisterRoutine(&help);
}

int RoutineLibExecute(int argc, char** argv)
{
    if (argc == 0)
    {
        puts("RoutineLib: Nothing to do. Use action `Help` to display all available actions.");
        return 0;
    }

    // requested routine's name
    char* rqroutname = argv[0];

    argv++;
    argc--;

    for (size_t i = 0; i < g_szRoutines; i++)
    {
        Routine* routine = g_pRoutines + i;
        if (strcmp(rqroutname, routine->Name) == 0)
        {
            if (argc < routine->MinParameters)
            {
                printf("RoutineLib: Routine `%s` requires at least %zu parameters but only %d were provided.\n", rqroutname, routine->MinParameters, argc);
                return EXIT_FAILURE;
            }
            if (argc > routine->NumberOfParameters)
            {
                printf("RoutineLib: Routine `%s` accepts at most %zu parameters but over %d were provided.\n", rqroutname, routine->NumberOfParameters, argc);
                return EXIT_FAILURE;
            }
            for (int i = 0; i < argc; i++)
            {
                RoutineParameter* param = routine->Parameters + i;
                switch (param->Type)
                {
                case ROUTINE_PARAMETER_INTEGER:
                {
                    char* endptr;
                    long val = strtol(argv[i], &endptr, 10);
                    if (endptr == argv[i])
                    {
                        printf("RoutineLib: Parameter No.%d of routine `%s` is supposed to be an integer.\n", i+1, rqroutname);
                        return EXIT_FAILURE;
                    }
                    param->IntValue = (int) val;
                    break;
                }
                case ROUTINE_PARAMETER_STRING:
                {
                    param->StrValue = argv[i];
                    break;
                }
                case ROUTINE_PARAMETER_BOOLEAN:
                {
                    if (strcmp(argv[i], "False") == 0)
                    {
                        param->BitValue = false;
                    }
                    else if (strcmp(argv[i], "True") == 0)
                    {
                        param->BitValue = true;
                    }
                    else
                    {
                        printf("RoutineLib: Parameter No.%d of routine `%s` is supposed to be a boolean, therefore only accepts the values `False` and `True`.\n", i+1, rqroutname);
                    }
                    break;
                }
                }
            }
            
            int result = routine->Handler(routine);
            switch (result)
            {
            case ROUTINE_OK:
            {
                printf("RoutineLib: The action `%s` has been completed successfully.\n", rqroutname);
                return EXIT_SUCCESS;
            }
            case ROUTINE_NOT_IMPLEMENTED:
            {
                printf("RoutineLib: The action `%s` is not implemented yet.\n", rqroutname);
                return EXIT_FAILURE;
            }
            default: break;
            }

            printf("RoutineLib: The action `%s` has failed with the return code %d.\n", rqroutname, result);
            return EXIT_FAILURE;
        }
    }

    printf("RoutineLib: No such action as `%s` exists. Use the `Help` action to display all available actions.\n", rqroutname);
    return EXIT_FAILURE;
}

bool RegisterRoutine(Routine* routine)
{
    if (!g_pRoutines)
    {
        if (!(g_pRoutines = malloc(sizeof(Routine))))
        {
            return false;
        }
    }
    else if (!(g_pRoutines = realloc(g_pRoutines, g_szRoutines * sizeof(Routine) + sizeof(Routine))))
    {
        return false;
    }

    g_pRoutines[g_szRoutines] = *routine; // copy
    return FinalizeRoutine(g_pRoutines + g_szRoutines++, routine->Usage == NULL);
}

Routine* GetRegisteredRoutines(size_t* pOutNumberOfRoutines)
{
    if (pOutNumberOfRoutines)
    {
        *pOutNumberOfRoutines = g_szRoutines;
    }
    return g_pRoutines;
}

RoutineParameter MakeIntegerRoutineParameter(const char* name, const char* desc) {
    return (RoutineParameter) { .Type = ROUTINE_PARAMETER_INTEGER, .Name = name, .Description = desc, .Optional = false };
}
RoutineParameter MakeStringRoutineParameter(const char* name, const char* desc) {
    return (RoutineParameter) { .Type = ROUTINE_PARAMETER_STRING, .Name = name, .Description = desc, .Optional = false };
}
RoutineParameter MakeBooleanRoutineParameter(const char* name, const char* desc) {
    return (RoutineParameter) { .Type = ROUTINE_PARAMETER_BOOLEAN, .Name = name, .Description = desc, .Optional = false };
}

RoutineParameter MakeOptIntegerRoutineParameter(const char* name, const char* desc, int32_t defaultValue) {
    return (RoutineParameter) { .Type = ROUTINE_PARAMETER_INTEGER, .Name = name, .Description = desc, .Optional = true, .IntValue = defaultValue };
}
RoutineParameter MakeOptStringRoutineParameter(const char* name, const char* desc, const char* defaultValue) {
    return (RoutineParameter) { .Type = ROUTINE_PARAMETER_STRING, .Name = name, .Description = desc, .Optional = true, .StrValue = defaultValue };
}
RoutineParameter MakeOptBooleanRoutineParameter(const char* name, const char* desc, bool defaultValue) {
    return (RoutineParameter) { .Type = ROUTINE_PARAMETER_BOOLEAN, .Name = name, .Description = desc, .Optional = true, .BitValue = defaultValue };
}

static const char* ParamTypeToStr(RoutineParameterType type)
{
    switch (type)
    {
    case ROUTINE_PARAMETER_INTEGER: return "Int";
    case ROUTINE_PARAMETER_STRING:  return "Str";
    case ROUTINE_PARAMETER_BOOLEAN: return "Bool";
    default:                        break;
    }
    return NULL;
}

Routine MakeRoutine(int (*handler)(const Routine* pSelf), const char* name, const char* desc)
{
    return (Routine) { .Handler = handler, .Name = name, .Description = desc, .Usage = NULL, .Parameters = NULL, .NumberOfParameters = 0 };
}
bool AddRoutineParameter(Routine* pRoutine, RoutineParameter param)
{
    if (!pRoutine->Parameters)
    {
        if (!(pRoutine->Parameters = malloc(sizeof(RoutineParameter))))
        {
            return false;
        }
    }
    else if (!(pRoutine->Parameters = realloc(pRoutine->Parameters, pRoutine->NumberOfParameters * sizeof(RoutineParameter) + sizeof(RoutineParameter))))
    {
        return false;
    }
    pRoutine->Parameters[pRoutine->NumberOfParameters++] = param;
    return true;
}
bool FinalizeRoutine(Routine* pRoutine, bool populateUsage)
{
    bool foundOpt = false;
    size_t szPrevStageStr = 0;
    for (size_t i = 0; i < pRoutine->NumberOfParameters; i++)
    {
        RoutineParameter* param = &pRoutine->Parameters[i];
        if (foundOpt)
        {
            if (!param->Optional)
            {
                printf("RoutineLib.FinalizeRoutine: Parameter at index %zu of routine `%s` was marked as not optional, but comes after an optional parameter.", i, pRoutine->Name);
                return false;
            }
        }
        else if (param->Optional)
        {
            pRoutine->MinParameters = i;
            foundOpt = true;
        }

        // This is coded horribly I know, I just want to get it over with already. We need to code an OS still
        if (populateUsage)
        {
            const char* format = "[%s:%s](%s) ";
            if (param->Optional)
            {
                switch (param->Type)
                {
                case ROUTINE_PARAMETER_INTEGER: format = "[%s:%s](%s)<Optional,ByDefault=%d> "; break;
                case ROUTINE_PARAMETER_BOOLEAN:
                case ROUTINE_PARAMETER_STRING:
                    format = "[%s:%s](%s)<Optional,ByDefault=%s> "; break;
                }
            }

            size_t len;
            if (param->Optional)
            {
                const char* strf = param->StrValue;
                switch (param->Type)
                {
                case ROUTINE_PARAMETER_INTEGER:
                {
                    len = snprintf(NULL, 0, format, param->Name, ParamTypeToStr(param->Type), param->Description, param->IntValue);
                    break;
                }
                case ROUTINE_PARAMETER_BOOLEAN:
                    strf = param->BitValue ? "True" : "False";
                    // falls through
                case ROUTINE_PARAMETER_STRING:
                    len = snprintf(NULL, 0, format, param->Name, ParamTypeToStr(param->Type), param->Description, strf);
                    break;
                }
            }
            else
            {
                len = snprintf(NULL, 0, format, param->Name, ParamTypeToStr(param->Type), param->Description);
            }

            char* dest = NULL;
            if (pRoutine->Usage)
            {
                pRoutine->Usage = realloc(pRoutine->Usage, szPrevStageStr + len + 1);
                dest = pRoutine->Usage + szPrevStageStr;
            }
            else
            {
                dest = pRoutine->Usage = malloc(len + 1);
            }
            szPrevStageStr += len;

            if (param->Optional)
            {
                const char* strf = param->StrValue;
                switch (param->Type)
                {
                case ROUTINE_PARAMETER_INTEGER:
                {
                    snprintf(dest, len + 1, format, param->Name, ParamTypeToStr(param->Type), param->Description, param->IntValue);
                    break;
                }
                case ROUTINE_PARAMETER_BOOLEAN:
                    strf = param->BitValue ? "True" : "False";
                    // falls through
                case ROUTINE_PARAMETER_STRING:
                    snprintf(dest, len + 1, format, param->Name, ParamTypeToStr(param->Type), param->Description, strf);
                    break;
                }
            }
            else
            {
                snprintf(dest, len + 1, format, param->Name, ParamTypeToStr(param->Type), param->Description);                
            }
        }
    }

    if (populateUsage && pRoutine->NumberOfParameters == 0)
    {
        pRoutine->Usage = "As Is (Provide No Parameters)";
    }
    if (pRoutine->NumberOfParameters != 0 && pRoutine->MinParameters == 0)
    {
        pRoutine->MinParameters = pRoutine->NumberOfParameters;
    }

    return true;
}
