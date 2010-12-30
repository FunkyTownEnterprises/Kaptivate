// Kaptivate.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "Kaptivate.h"


// This is an example of an exported variable
KAPTIVATE_API int nKaptivate=0;

// This is an example of an exported function.
KAPTIVATE_API int fnKaptivate(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see Kaptivate.h for the class definition
CKaptivate::CKaptivate()
{
	return;
}
