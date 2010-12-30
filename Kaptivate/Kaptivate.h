// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the KAPTIVATE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// KAPTIVATE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef KAPTIVATE_EXPORTS
#define KAPTIVATE_API __declspec(dllexport)
#else
#define KAPTIVATE_API __declspec(dllimport)
#endif

// This class is exported from the Kaptivate.dll
class KAPTIVATE_API CKaptivate {
public:
	CKaptivate(void);
	// TODO: add your methods here.
};

extern KAPTIVATE_API int nKaptivate;

KAPTIVATE_API int fnKaptivate(void);
