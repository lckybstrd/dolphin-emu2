#ifdef __cplusplus
extern "C" {
#endif

#ifndef SCRIPT_CONTEXT
#define SCRIPT_CONTEXT


#include "Core/Scripting/CoreScriptContextFiles/InstructionBreakpointsHolder.h"
#include "Core/Scripting/CoreScriptContextFiles/MemoryAddressBreakpointsHolder.h"
#include "Core/Scripting/CoreScriptContextFiles/ScriptCallLocations.h"

extern const char* most_recent_script_version;

struct ScriptContext;

typedef struct ScriptContextBaseFunctionsTable
{

  void (*print_callback)(struct ScriptContext*, const char*);
  void (*script_end_callback)(struct ScriptContext*, int);

  void (*ImportModule)(struct ScriptContext*, const char*, const char*);

  void (*StartScript)(struct ScriptContext*);
  void (*RunGlobalScopeCode)(struct ScriptContext*);

  void (*RunOnFrameStartCallbacks)(struct ScriptContext*);
  void (*RunOnGCControllerPolledCallbacks)(struct ScriptContext*);
  void (*RunOnInstructionReachedCallbacks)(struct ScriptContext*, unsigned int);
  void (*RunOnMemoryAddressReadFromCallbacks)(struct ScriptContext*, unsigned int);
  void (*RunOnMemoryAddressWrittenToCallbacks)(struct ScriptContext*, unsigned int);
  void (*RunOnWiiInputPolledCallbacks)(struct ScriptContext*);

  void* (*RegisterOnFrameStartCallbacks)(struct ScriptContext*, void*);
  void (*RegisterOnFrameStartWithAutoDeregistrationCallbacks)(struct ScriptContext*, void*);
  int (*UnregisterOnFrameStartCallbacks)(struct ScriptContext*, void*);

  void* (*RegisterOnGCControllerPolledCallbacks)(struct ScriptContext*, void*);
  void (*RegisterOnGCControllerPolledWithAutoDeregistrationCallbacks)(struct ScriptContext*, void*);
  int (*UnregisterOnGCControllerPolledCallbacks)(struct ScriptContext*, void*);

  void* (*RegisterOnInstructionReachedCallbacks)(struct ScriptContext*, unsigned int, void*);
  void (*RegisterOnInstructioReachedWithAutoDeregistrationCallbacks)(struct ScriptContext*, unsigned int, void*);
  int (*UnregisterOnInstructionReachedCallbacks)(struct ScriptContext*, unsigned int, void*);

  void* (*RegisterOnMemoryAddressReadFromCallbacks)(struct ScriptContext*, unsigned int, void*);
  void (*RegisterOnMemoryAddressReadFromWithAutoDeregistrationCallbacks)(struct ScriptContext*, unsigned int, void*);
  int (*UnregisterOnMemoryAddressReadFromCallbacks)(struct ScriptContext*, unsigned int, void*);

  void* (*RegisterOnMemoryAddressWrittenToCallbacks)(struct ScriptContext*, unsigned int, void*);
  void (*RegisterOnMemoryAddressWrittenToWithAutoDeregistrationCallbacks)(struct ScriptContext*, unsigned int, void*);
  int (*UnregisterOnMemoryAddressWrittenToCallbacks)(struct ScriptContext*, unsigned int, void*);

  void* (*RegisterOnWiiInputPolledCallbacks)(struct ScriptContext*, void*);
  void(*RegisterOnWiiInputPolledWithAutoDeregistrationCallbacks) (struct ScriptContext*, void*);
  int (*UnregisterOnWiiInputPolledCallbacks)(struct ScriptContext*, void*);

  void (*RegisterButtonCallback)(struct ScriptContext*, long long, void*);
  int (*IsButtonRegistered)(struct ScriptContext*, long long);
  void (*GetButtonCallbackAndAddToQueue)(struct ScriptContext*, long long);
  void (*RunButtonCallbacksInQueue)(struct ScriptContext*);

} ScriptContextBaseFunctionsTable;


typedef struct ScriptContext
{
  int unique_script_identifier;
  const char* script_filename;
  ScriptCallLocations current_script_call_location;
  int is_script_active;
  int finished_with_global_code;
  int called_yielding_function_in_last_global_script_resume;
  int called_yielding_function_in_last_frame_callback_script_resume;
  void* script_specific_lock;

  InstructionBreakpointsHolder instructionBreakpointsHolder;
  MemoryAddressBreakpointsHolder memoryAddressBreakpointsHolder;
  ScriptContextBaseFunctionsTable scriptContextBaseFunctionsTable;
  ScriptContext* (*Initialize_ScriptContext)(int, const char*, void (*)(const char*), void (*)(int));

} ScriptContext;

ScriptContext* CreateScript(int unique_identifier, const char* script_file_name, void (*print_callback_function)(const char*), void(*script));
void ShutdownScript(ScriptContext* script_context);

#endif

#ifdef __cplusplus
}
#endif
