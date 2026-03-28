#ifndef YCH_TOOLS_ROUTINELIB_H
#define YCH_TOOLS_ROUTINELIB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ROUTINE_OK              0
#define ROUTINE_FAIL            1
#define ROUTINE_NOT_IMPLEMENTED 2

#define EXTERN_ROUTINE_HANDLER(handler_name) extern int handler_name(const Routine* pSelf)

typedef enum
{
    ROUTINE_PARAMETER_INTEGER,
    ROUTINE_PARAMETER_STRING,
    ROUTINE_PARAMETER_BOOLEAN // False/True
} RoutineParameterType;

typedef struct
{
    RoutineParameterType Type;
    const char*          Name;
    const char*          Description;
    
    bool                 Optional;
    int32_t              IntValue;
    const char*          StrValue;
    bool                 BitValue;
} RoutineParameter;

RoutineParameter MakeIntegerRoutineParameter(const char* name, const char* desc);
RoutineParameter MakeStringRoutineParameter(const char* name, const char* desc);
RoutineParameter MakeBooleanRoutineParameter(const char* name, const char* desc);

RoutineParameter MakeOptIntegerRoutineParameter(const char* name, const char* desc, int32_t defaultValue);
RoutineParameter MakeOptStringRoutineParameter(const char* name, const char* desc, const char* defaultValue);
RoutineParameter MakeOptBooleanRoutineParameter(const char* name, const char* desc, bool defaultValue);

typedef struct Routine
{
    const char* Name;
    const char* Description;
          char* Usage;
    
    RoutineParameter* Parameters;
    size_t NumberOfParameters;
    size_t MinParameters;
    
    int (*Handler)(const struct Routine* pSelf);
} Routine;

Routine MakeRoutine(int (*handler)(const Routine* pSelf), const char* name, const char* desc);
bool    AddRoutineParameter(Routine* pRoutine, RoutineParameter param);
bool    FinalizeRoutine(Routine* pRoutine, bool populateUsage);

bool RoutineLibInit(void);
void RoutineLibShutdown(void);
// RegisterRoutine will call FinalizeRoutine if `Usage == NULL`.
bool RegisterRoutine(Routine* routine);
Routine* GetRegisteredRoutines(size_t* pOutNumberOfRoutines);

int RoutineLibExecute(int argc, char** argv);

#endif // !YCH_TOOLS_ROUTINELIB_H
